#include "pch.hpp"
#include "offline_library.hpp"

#include "util/files.hpp"
#include "util/log_channels.hpp"

namespace qs
{
	namespace
	{
		Poco::Logger& logger()
		{
			return Poco::Logger::get(channel::OfflineStreaming);
		}

		std::string trackKey(const std::string_view name, const std::string_view bitrate)
		{
			return std::format("{}_{}", name, bitrate);
		}

		// Audio and text streams name their track in a param, while the video stream is always "video".
		std::string trackName(const Poco::XML::Element& media, const std::string_view tag)
		{
			if (tag != "video")
			{
				const Poco::AutoPtr<Poco::XML::NodeList> params = media.getElementsByTagName("param");
				for (unsigned long i = 0; i < params->length(); ++i)
				{
					const auto* param = static_cast<Poco::XML::Element*>(params->item(i));
					if (param->getAttribute("name") == "trackName")
						return param->getAttribute("value");
				}
			}

			return "video";
		}
	}

	OfflineLibrary::OfflineLibrary(const std::string& episodesPath, const std::vector<std::string>& episodeIds)
	{
		logger().information("Initializing offline playback subsystem...");

		for (const std::string& episodeId : episodeIds)
		{
			const std::optional<Poco::Path> folder = findEpisodeFolder(episodesPath, episodeId);
			if (!folder)
				continue;

			for (Poco::DirectoryIterator file(*folder), end; file != end; ++file)
			{
				if (file.path().getExtension() != "ism")
					continue;

				if (std::optional<Episode> episode = loadEpisode(episodeId, *folder, file.path()))
					episodes_[episodeId] = std::move(*episode);
			}
		}

		logger().information(std::format("Found {} stored episodes", episodes_.size()));
	}

	std::optional<OfflineLibrary::Episode> OfflineLibrary::loadEpisode(const std::string& episodeId,
	                                                                   const Poco::Path& folder,
	                                                                   const Poco::Path& serverManifest)
	{
		const std::optional<std::string> xml = readFile(serverManifest.toString());
		if (!xml)
		{
			logger().error(std::format("Failed to open server manifest file ({}) for episode {}",
			                           serverManifest.getFileName(), episodeId));
			return std::nullopt;
		}

		try
		{
			Poco::XML::DOMParser parser;
			const Poco::AutoPtr<Poco::XML::Document> document = parser.parseString(*xml);

			const auto* meta = dynamic_cast<Poco::XML::Element*>(
				document->getNodeByPath("//head/meta[@name='clientManifestRelativePath']"));
			if (!meta)
			{
				logger().warning(std::format("Server manifest file ({}) missing clientManifestRelativePath, skipping.",
				                             serverManifest.getFileName()));
				return std::nullopt;
			}

			Episode episode{.clientManifest = resolvePath(folder, meta->getAttribute("content")), .tracks = {}};

			for (const std::string_view tag : {"video", "audio", "textstream"})
			{
				const Poco::AutoPtr<Poco::XML::NodeList> nodes = document->getElementsByTagName(std::string(tag));
				for (unsigned long i = 0; i < nodes->length(); ++i)
				{
					const auto* media = static_cast<Poco::XML::Element*>(nodes->item(i));
					const std::string name = trackName(*media, tag);
					const std::string bitrate = media->getAttribute("systemBitrate");
					const Poco::Path file = resolvePath(folder, media->getAttribute("src"));

					if (std::optional<Track> track = loadTrack(file))
					{
						episode.tracks[trackKey(name, bitrate)] = std::move(*track);
						logger().debug(std::format("Preloaded {} track '{}' for episode {} from {} with bitrate {}", tag,
						                           name, episodeId, file.toString(), bitrate));
					}
				}
			}

			return episode;
		}
		catch (const Poco::Exception& error)
		{
			logger().warning(std::format("Cannot read server manifest file ({}) for episode {} ({}), skipping.",
			                             serverManifest.getFileName(), episodeId, error.displayText()));
			return std::nullopt;
		}
	}

	std::optional<OfflineLibrary::Track> OfflineLibrary::loadTrack(const Poco::Path& file)
	{
		std::ifstream stream(file.toString(), std::ios::binary);
		if (!stream)
		{
			logger().warning(std::format("Failed to open track file: {}", file.toString()));
			return std::nullopt;
		}

		try
		{
			return Track{.file = file, .fragments = mp4::readFragmentIndex(stream)};
		}
		catch (const mp4::FormatError& error)
		{
			logger().warning(std::format("Invalid fragment index in track file {} ({}), skipping this track.",
			                             file.toString(), error.what()));
			return std::nullopt;
		}
	}

	std::optional<std::string> OfflineLibrary::clientManifest(const std::string_view episodeId) const
	{
		const auto episode = episodes_.find(episodeId);
		if (episode == episodes_.end())
			return std::nullopt;

		const std::string path = episode->second.clientManifest.toString();
		std::optional<std::string> manifest = readFile(path);
		if (!manifest)
			logger().warning(std::format("Cannot open the client manifest {}, although it was there at startup", path));

		return manifest;
	}

	std::optional<std::string> OfflineLibrary::fragment(const std::string_view episodeId, const std::string_view track,
	                                                    const std::string_view bitrate,
	                                                    const std::uint64_t startTime) const
	{
		const auto episode = episodes_.find(episodeId);
		if (episode == episodes_.end())
			return std::nullopt;

		const auto media = episode->second.tracks.find(trackKey(track, bitrate));
		if (media == episode->second.tracks.end())
			return std::nullopt;

		const auto entry = media->second.fragments.find(startTime);
		if (entry == media->second.fragments.end())
			return std::nullopt;

		const std::string path = media->second.file.toString();
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			logger().warning(std::format("Cannot open the track file {}, although it was there at startup", path));
			return std::nullopt;
		}

		try
		{
			return mp4::readFragment(stream, entry->second);
		}
		catch (const mp4::FormatError& error)
		{
			logger().warning(std::format("Invalid fragment at start time {} in track {} ({})", startTime, path,
			                             error.what()));
			return std::nullopt;
		}
	}
}
