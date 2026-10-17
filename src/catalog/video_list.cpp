#include "pch.hpp"
#include "video_list.hpp"

#include "app/logging.hpp"
#include "util/files.hpp"

namespace qs
{
	namespace
	{
		constexpr auto kGameVideoListPath = "./data/videoList.rmdj";
		constexpr std::string_view kManifestSegment = "manifest";

		constexpr std::array<unsigned char, 32> kCipherKey{
			0xba, 0x7a, 0xbb, 0x27, 0x03, 0x9b, 0x72, 0xfd, 0x13, 0xeb, 0x70, 0x38, 0x7e, 0x0f, 0xcb, 0x41,
			0xe1, 0xd0, 0xeb, 0x54, 0xbe, 0x8f, 0x13, 0x6d, 0xf0, 0xba, 0xe2, 0x2a, 0xdc, 0xfb, 0x40, 0xf1
		};

		Poco::Logger& logger()
		{
			return Poco::Logger::get(channel::VideoList);
		}

		bool writeFile(const std::string& path, std::string_view data)
		{
			std::ofstream stream(path, std::ios::binary | std::ios::trunc);
			stream.write(data.data(), static_cast<std::streamsize>(data.size()));
			stream.close();
			return !stream.fail();
		}
	}

	void applyRmdjCipher(std::string& data)
	{
		for (std::size_t i = 0; i < data.size(); ++i)
			data[i] = static_cast<char>(data[i] ^ kCipherKey[i % kCipherKey.size()]);
	}

	VideoList VideoList::load(const std::string& path)
	{
		std::optional<std::string> data = readFile(path);
		if (!data)
		{
			logger().error(std::format("Cannot read the video list {}, no episodes can be loaded!", path));
			return {};
		}

		applyRmdjCipher(*data);

		VideoList list;
		try
		{
			const auto object = Poco::JSON::Parser().parse(*data).extract<Poco::JSON::Object::Ptr>();
			for (const auto& [episodeId, url] : *object)
				list.manifestUrls_.emplace(episodeId, url.convert<std::string>());
		}
		catch (const Poco::Exception& error)
		{
			logger().error(std::format("Cannot parse the video list {} ({}), no episodes can be loaded!", path,
			                           error.displayText()));
			return {};
		}

		logger().information(std::format("Successfully loaded videos list! (Videos count: {})",
		                                 list.manifestUrls_.size()));
		return list;
	}

	VideoList VideoList::patchGameFile(const std::string& originalPath, const unsigned short port)
	{
		try
		{
			if (!Poco::File(originalPath).exists())
			{
				logger().warning(std::format("Copying video list file from {} to {}...", kGameVideoListPath,
				                             originalPath));
				Poco::File(kGameVideoListPath).copyTo(originalPath);
			}
		}
		catch (const Poco::Exception& error)
		{
			logger().error(std::format("No video list file found to patch ({})", error.displayText()));
			return {};
		}

		VideoList original = load(originalPath);
		// An empty list would leave the game without episodes, so the game's file is only replaced by a real one.
		if (original.manifestUrls_.empty())
			return original;

		Poco::JSON::Object patched;
		for (const std::string& episodeId : original.manifestUrls_ | std::views::keys)
			patched.set(episodeId, std::format("http://127.0.0.1:{}/{}/manifest", port, episodeId));

		std::ostringstream json;
		patched.stringify(json, 4);
		std::string data = json.str();
		applyRmdjCipher(data);

		if (writeFile(kGameVideoListPath, data))
			logger().information(std::format("Video list file patched successfully! (Videos count: {})",
			                                 patched.size()));
		else
			logger().error(std::format("Failed to write the patched video list to {}", kGameVideoListPath));

		return original;
	}

	std::vector<std::string> VideoList::episodeIds() const
	{
		const auto ids = manifestUrls_ | std::views::keys;
		return {ids.begin(), ids.end()};
	}

	std::optional<std::string> VideoList::manifestUrl(const std::string_view episodeId) const
	{
		if (const auto entry = manifestUrls_.find(episodeId); entry != manifestUrls_.end())
			return entry->second;

		return std::nullopt;
	}

	std::optional<std::string> VideoList::fragmentUrl(const std::string_view episodeId, const std::string_view bitrate,
	                                                  const std::string_view track,
	                                                  const std::string_view startTime) const
	{
		std::optional<std::string> url = manifestUrl(episodeId);
		if (!url)
			return std::nullopt;

		const std::size_t position = url->find(kManifestSegment);
		if (position == std::string::npos)
		{
			logger().warning(std::format("Failed to find '{}' in URL: {}", kManifestSegment, *url));
			return std::nullopt;
		}

		url->replace(position, kManifestSegment.size(),
		             std::format("QualityLevels({})/Fragments({}={})", bitrate, track, startTime));
		return url;
	}
}
