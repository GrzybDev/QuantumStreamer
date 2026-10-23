#include <memory>

#include <gtest/gtest.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/StreamCopier.h>

#include "catalog/video_list.hpp"
#include "offline/offline_library.hpp"
#include "server/handler_factory.hpp"
#include "subtitles/subtitle_overrides.hpp"
#include "support.hpp"

namespace qs::test
{
	namespace
	{
		constexpr auto kServerManifest =
			R"(<?xml version="1.0" encoding="utf-8"?><smil xmlns="http://www.w3.org/2001/SMIL20/Language">)"
			R"(<head><meta name="clientManifestRelativePath" content="E1.ismc"/></head><body><switch>)"
			R"(<video src="E1_video.ismv" systemBitrate="512000"/></switch></body></smil>)";

		struct Answer
		{
			int status;
			std::string body;
		};
	}

	// Runs the real server over loopback in offline mode, so nothing reaches the CDN.
	class ServerTest : public testing::Test
	{
	protected:
		ServerTest()
		{
			folder_.write("list.rmdj", encryptedVideoList(R"({"E1": "http://cdn.example/E1.ism/manifest"})"));
			folder_.write("episodes/E1/E1.ism", kServerManifest);
			folder_.write("episodes/E1/E1.ismc", "<SmoothStreamingMedia/>");
			folder_.write("episodes/E1/E1_video.ismv", trackFile({{0, fragment("frame")}}));

			const std::string episodes = (folder_.path() / "episodes").string();
			videoList_ = std::make_unique<VideoList>(VideoList::load((folder_.path() / "list.rmdj").string()));
			library_ = std::make_unique<OfflineLibrary>(episodes, videoList_->episodeIds());
			subtitles_ = std::make_unique<SubtitleOverrides>(episodes, videoList_->episodeIds(), SubtitleOptions{});

			server_ = std::make_unique<Poco::Net::HTTPServer>(
				new RequestHandlerFactory({*videoList_, *library_, *subtitles_, true}), socket_,
				new Poco::Net::HTTPServerParams);
			server_->start();
		}

		~ServerTest() override
		{
			server_->stop();
		}

		Answer request(const std::string& path, const std::string& method = Poco::Net::HTTPRequest::HTTP_GET)
		{
			Poco::Net::HTTPClientSession session(kLocalServerHost, socket_.address().port());
			Poco::Net::HTTPRequest request(method, path, Poco::Net::HTTPMessage::HTTP_1_1);
			session.sendRequest(request);

			Poco::Net::HTTPResponse response;
			std::string body;
			Poco::StreamCopier::copyToString(session.receiveResponse(response), body);
			return {.status = response.getStatus(), .body = body};
		}

		TempFolder folder_;
		Poco::Net::ServerSocket socket_{Poco::Net::SocketAddress(kLocalServerHost, 0)};
		std::unique_ptr<VideoList> videoList_;
		std::unique_ptr<OfflineLibrary> library_;
		std::unique_ptr<SubtitleOverrides> subtitles_;
		std::unique_ptr<Poco::Net::HTTPServer> server_;
	};

	TEST_F(ServerTest, ServesTheStoredClientManifest)
	{
		const Answer answer = request("/E1/manifest");

		EXPECT_EQ(answer.status, 200);
		EXPECT_EQ(answer.body, "<SmoothStreamingMedia/>");
	}

	TEST_F(ServerTest, ServesAStoredFragment)
	{
		const Answer answer = request("/E1/QualityLevels(512000)/Fragments(video=0)");

		EXPECT_EQ(answer.status, 200);
		EXPECT_EQ(answer.body, fragment("frame"));
	}

	TEST_F(ServerTest, RefusesAFragmentThatIsNotStoredWhileOffline)
	{
		EXPECT_EQ(request("/E1/QualityLevels(512000)/Fragments(video=20020000)").status, 406);
		EXPECT_EQ(request("/E1/QualityLevels(256000)/Fragments(enus=0)").status, 406);
	}

	TEST_F(ServerTest, AnswersNotFoundForUnknownEpisodesAndPaths)
	{
		EXPECT_EQ(request("/E9/manifest").status, 404);
		EXPECT_EQ(request("/E1/manifest/extra").status, 404);
	}

	TEST_F(ServerTest, AnswersNotImplementedForOtherMethods)
	{
		EXPECT_EQ(request("/E1/manifest", Poco::Net::HTTPRequest::HTTP_POST).status, 501);
	}
}
