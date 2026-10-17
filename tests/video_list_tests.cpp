#include <filesystem>

#include <gtest/gtest.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include "catalog/video_list.hpp"
#include "support.hpp"

namespace qs::test
{
	namespace
	{
		constexpr auto kGameFile = "data/videoList.rmdj";
		constexpr auto kOriginalFile = "data/videoList_original.rmdj";
		constexpr auto kOriginalPath = "./data/videoList_original.rmdj";

		const std::string kStoreList = encryptedVideoList(
			R"({"E1": "http://cdn.example/E1.ism/manifest?token=1", "E2": "http://cdn.example/E2.ism/manifest?token=2"})");

		std::string decrypt(std::string data)
		{
			applyRmdjCipher(data);
			return data;
		}
	}

	// The game's own list has a fixed path relative to the game folder, so each test runs in a folder of its own.
	class VideoListTest : public testing::Test
	{
	protected:
		VideoListTest() : previous_(std::filesystem::current_path())
		{
			std::filesystem::current_path(folder_.path());
		}

		~VideoListTest() override
		{
			std::filesystem::current_path(previous_);
		}

		TempFolder folder_;
		std::filesystem::path previous_;
	};

	TEST_F(VideoListTest, PatchesTheGameListAndKeepsTheStoresCopy)
	{
		folder_.write(kGameFile, kStoreList);

		const VideoList original = VideoList::patchGameFile(kOriginalPath, 1234);

		EXPECT_EQ(original.manifestUrl("E1"), "http://cdn.example/E1.ism/manifest?token=1");
		EXPECT_EQ(folder_.read(kOriginalFile), kStoreList);

		const auto patched = Poco::JSON::Parser().parse(decrypt(folder_.read(kGameFile)))
		                                         .extract<Poco::JSON::Object::Ptr>();
		EXPECT_EQ(patched->getValue<std::string>("E1"), "http://127.0.0.1:1234/E1/manifest");
		EXPECT_EQ(patched->getValue<std::string>("E2"), "http://127.0.0.1:1234/E2/manifest");
	}

	TEST_F(VideoListTest, BuildsFragmentUrlsKeepingTheQuery)
	{
		folder_.write(kOriginalFile, kStoreList);

		const VideoList list = VideoList::load(kOriginalPath);

		EXPECT_EQ(list.episodeIds(), (std::vector<std::string>{"E1", "E2"}));
		EXPECT_EQ(list.fragmentUrl("E2", "256000", "enus_captions", "20020000"),
		          "http://cdn.example/E2.ism/QualityLevels(256000)/Fragments(enus_captions=20020000)?token=2");
		EXPECT_FALSE(list.fragmentUrl("E3", "256000", "enus", "0").has_value());
	}

	TEST_F(VideoListTest, LeavesTheGameListAloneWhenTheCopyIsUnreadable)
	{
		folder_.write(kGameFile, kStoreList);
		folder_.write(kOriginalFile, "not a video list");

		EXPECT_TRUE(VideoList::patchGameFile(kOriginalPath, 1234).episodeIds().empty());
		EXPECT_EQ(folder_.read(kGameFile), kStoreList);
	}
}
