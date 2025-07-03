#include "pch.hpp"
#include "offline_streaming.hpp"

#include "video_list.hpp"

using Poco::AutoPtr;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Logger;
using Poco::Path;
using Poco::Util::Application;
using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::InputSource;
using Poco::XML::Node;
using Poco::XML::NodeList;

const char* OfflineStreaming::name() const
{
	return "OfflineStreaming";
}

void OfflineStreaming::initialize(Application& app)
{
	Logger& logger = Logger::get(name());
	logger.information("Initializing offline playback subsystem...");

	std::string episodesPath = app.config().getString("Server.EpisodesPath", "./videos/episodes");
	VideoList& videoList = app.getSubsystem<VideoList>();

	// Check if the episodes path exists
	for (auto episodes = videoList.getEpisodeList(); const auto& episode : episodes)
	{
		Path episodePath(episodesPath);
		episodePath.append(episode);
		File episodeDir(episodePath);

		if (!(episodeDir.exists() && episodeDir.isDirectory()))
			continue;

		// Find all *.ism files in the episode directory
		for (DirectoryIterator it(episodeDir), end; it != end; ++it)
		{
			if (Path(it.name()).getExtension() != "ism")
				continue;

			std::ifstream fileStream(it.path().toString());
			if (!fileStream)
			{
				logger.error("Failed to open server manifest file (%s) for episode %s", it.name(), episode);
				continue;
			}

			std::string manifestContent((std::istreambuf_iterator<char>(fileStream)), {});
			std::istringstream manifestStream(manifestContent);
			InputSource manifestSource(manifestStream);
			DOMParser parser;
			AutoPtr doc(parser.parse(&manifestSource));

			SmoothStream stream;
			Node* metaNode = doc->getNodeByPath("//head/meta[@name='clientManifestRelativePath']");
			if (!metaNode || metaNode->nodeType() != Node::ELEMENT_NODE)
			{
				logger.warning("Server manifest file (%s) missing clientManifestRelativePath, skipping.", it.name());
				continue;
			}

			auto* metaElem = dynamic_cast<Element*>(metaNode);
			Path clientManifestPath = episodePath;
			clientManifestPath.append(metaElem->getAttribute("content"));
			stream.client_manifest_relative_path = clientManifestPath;

			processMediaNodes("video", doc, episode, episodePath.toString(), stream);
			processMediaNodes("audio", doc, episode, episodePath.toString(), stream);
			processMediaNodes("textstream", doc, episode, episodePath.toString(), stream);

			streams_[episode] = stream;
		}
	}

	logger.information("%s episodes are ready to offline playback!",
	                   std::to_string(streams_.size()));
}

void OfflineStreaming::uninitialize()
{
	streams_.clear();
}

void OfflineStreaming::processMediaNodes(const std::string& tag_name, Document* doc, const std::string& episode_id,
                                         const std::string& episode_path, SmoothStream& stream) const
{
	Logger& logger = Logger::get(name());

	NodeList* nodes = doc->getElementsByTagName(tag_name);
	for (unsigned long i = 0; i < nodes->length(); ++i)
	{
		auto* elem = dynamic_cast<Element*>(nodes->item(i));
		std::string src = elem->getAttribute("src");
		std::string bitrate = elem->getAttribute("systemBitrate");

		std::string trackName = "video";
		if (tag_name != "video")
		{
			NodeList* params = elem->getElementsByTagName("param");
			for (unsigned long j = 0; j < params->length(); ++j)
			{
				if (auto* param = dynamic_cast<Element*>(params->item(j)); param->getAttribute("name") == "trackName")
				{
					trackName = param->getAttribute("value");
					break;
				}
			}
		}

		Path fullPath = episode_path;
		fullPath.append(src);
		if (!fullPath.isFile())
			continue;

		std::string mediaKey = trackName + "_" + bitrate;

		SmoothMedia media;
		media.source_file = fullPath;
		media.system_bitrate = bitrate;

		if (auto [success, track] = preloadTrack(fullPath.toString()); success)
		{
			media.track = track;
			stream.media_map[mediaKey] = media;
			logger.debug("Preloaded %s track '%s' for episode %s from %s with bitrate %s", tag_name, trackName,
			             episode_id, fullPath.toString(), bitrate);
		}
	}
}

std::pair<bool, OfflineStreaming::SmoothTrack> OfflineStreaming::preloadTrack(const std::string& path) const
{
	Logger& logger = Logger::get(name());

	bool success = false;

	SmoothTrack track;

	std::ifstream trackStream(path, std::ios::binary);

	if (!trackStream)
	{
		logger.warning("Failed to open track file: %s", path);
		return {success, track};
	}

	// Read mfro box
	trackStream.seekg(-4, std::ios::end);

	unsigned int mfroSize;
	trackStream.read(reinterpret_cast<char*>(&mfroSize), sizeof(mfroSize));
	mfroSize = _byteswap_ulong(mfroSize);

	trackStream.seekg(-static_cast<int>(mfroSize), std::ios::end);

	// Read mfra box
	unsigned int mfraBlockSize;
	trackStream.read(reinterpret_cast<char*>(&mfraBlockSize), sizeof(mfraBlockSize));
	mfraBlockSize = _byteswap_ulong(mfraBlockSize);

	if (mfraBlockSize != mfroSize)
	{
		logger.warning("Invalid mfro block size in track file %s, expected %s, got: %s, skipping this track.", path,
		               std::to_string(mfroSize), std::to_string(mfraBlockSize));
		trackStream.close();

		return {success, track};
	}

	std::string mfraExpectedMagic = BLOCK_MFRA;
	std::string mfraMagic(mfraExpectedMagic.length(), '\0');
	trackStream.read(mfraMagic.data(), mfraExpectedMagic.length());

	if (mfraMagic != mfraExpectedMagic)
	{
		logger.warning("Invalid mfra magic in track file %s, expected: %s, got: %s, skipping this track.", path,
		               mfraExpectedMagic, mfraMagic);
		trackStream.close();

		return {success, track};
	}

	// Read tfra box
	unsigned int tfraSize;
	trackStream.read(reinterpret_cast<char*>(&tfraSize), sizeof(tfraSize));
	tfraSize = _byteswap_ulong(tfraSize);

	std::string tfraExpectedMagic = BLOCK_TFRA;
	std::string tfraMagic(tfraExpectedMagic.length(), '\0');
	trackStream.read(tfraMagic.data(), tfraExpectedMagic.length());

	if (tfraMagic != tfraExpectedMagic)
	{
		logger.warning("Invalid tfra magic in track file %s, expected: %s, got: %s, skipping this track.", path,
		               tfraExpectedMagic, tfraMagic);
		trackStream.close();

		return {success, track};
	}

	char version;
	trackStream.read(&version, sizeof(version));

	// Skip flags
	trackStream.seekg(FLAGS_TO_SKIP, std::ios::cur);

	unsigned int trackId;
	trackStream.read(reinterpret_cast<char*>(&trackId), sizeof(trackId));
	track.track_id = _byteswap_ulong(trackId);

	int temp;
	trackStream.read(reinterpret_cast<char*>(&temp), sizeof(temp));
	track.length_size_of_traf_num = ((temp & 0x3F) >> 4) + 1;
	track.length_size_of_trun_num = ((temp & 0xC) >> 2) + 1;
	track.length_size_of_sample_num = (temp & 0x3) + 1;

	unsigned int numberOfEntries;
	trackStream.read(reinterpret_cast<char*>(&numberOfEntries), sizeof(numberOfEntries));
	numberOfEntries = _byteswap_ulong(numberOfEntries);

	std::map<std::string, SmoothFragment> fragments;

	for (unsigned int i = 0; i < numberOfEntries; i++)
	{
		SmoothFragment fragment{};

		unsigned long long startTime;

		if (version == 1)
		{
			unsigned long long time;
			trackStream.read(reinterpret_cast<char*>(&time), sizeof(time));
			startTime = _byteswap_uint64(time);

			unsigned long long moofOffset;
			trackStream.read(reinterpret_cast<char*>(&moofOffset), sizeof(moofOffset));
			fragment.moof_offset = _byteswap_uint64(moofOffset);
		}
		else
		{
			unsigned int time;
			trackStream.read(reinterpret_cast<char*>(&time), sizeof(time));
			startTime = _byteswap_ulong(time);

			unsigned int moofOffset;
			trackStream.read(reinterpret_cast<char*>(&moofOffset), sizeof(moofOffset));
			fragment.moof_offset = _byteswap_ulong(moofOffset);
		}

		unsigned long long trafNumber;
		trackStream.read(reinterpret_cast<char*>(&trafNumber), track.length_size_of_traf_num);
		fragment.traf_number = _byteswap_uint64(trafNumber);

		unsigned long long trunNumber;
		trackStream.read(reinterpret_cast<char*>(&trunNumber), track.length_size_of_trun_num);
		fragment.trun_number = _byteswap_uint64(trunNumber);

		unsigned long long sampleNumber;
		trackStream.read(reinterpret_cast<char*>(&sampleNumber), track.length_size_of_sample_num);
		fragment.sample_number = _byteswap_uint64(sampleNumber);

		fragments[std::to_string(startTime)] = fragment;
	}

	track.fragments = fragments;
	success = true;
	trackStream.close();

	return {success, track};
}

std::string OfflineStreaming::getLocalClientManifest(const std::string& episode_id)
{
	if (!streams_.contains(episode_id))
		return "";

	Logger& logger = Logger::get(name());

	Path clientManifestRelativePath = streams_[episode_id].client_manifest_relative_path;
	std::ifstream clientManifestStream(clientManifestRelativePath.toString());

	if (!clientManifestStream)
	{
		logger.warning(
			"Failed to open client manifest file %s, the file was there while initializing, but it probably got deleted. Will need to fetch client manifest from server.",
			clientManifestRelativePath);
		clientManifestStream.close();

		return "";
	}

	std::stringstream buffer;
	buffer << clientManifestStream.rdbuf();

	clientManifestStream.close();

	return buffer.str();
}

std::string OfflineStreaming::getLocalFragment(const std::string& episode_id, const std::string& track_name,
                                               const std::string& bitrate,
                                               const std::string& start_time)
{
	if (!streams_.contains(episode_id))
		return {};

	Logger& logger = Logger::get(name());

	auto& [clientManifestRelativePath, mediaMap] = streams_[episode_id];
	std::string mediaKey = track_name + "_" + bitrate;

	if (!mediaMap.contains(mediaKey))
		return {};

	SmoothMedia media = mediaMap[mediaKey];
	SmoothTrack track = media.track;

	if (!track.fragments.contains(start_time))
		return {};

	SmoothFragment fragment = track.fragments[start_time];

	std::ifstream fragmentStream(media.source_file.toString(), std::ios::binary);

	if (!fragmentStream)
	{
		logger.warning(
			"Failed to open track file %s, the file was there while initializing, but it probably got deleted. Will need to fetch client manifest from server.",
			media.source_file.toString());
		fragmentStream.close();

		return {};
	}

	fragmentStream.seekg(static_cast<long long>(fragment.moof_offset));

	unsigned int moofSize;
	fragmentStream.read(reinterpret_cast<char*>(&moofSize), sizeof(moofSize));
	moofSize = _byteswap_ulong(moofSize);

	std::string moofExpectedMagic = BLOCK_MOOF;
	std::string moofMagic(moofExpectedMagic.length(), '\0');
	fragmentStream.read(moofMagic.data(), moofExpectedMagic.length());

	if (moofMagic != moofExpectedMagic)
	{
		logger.warning(
			"Invalid moof magic in fragment at start time %s in track %s, expected: %s, got %s. Will need to fetch that fragment from server.",
			start_time, media.source_file.toString(), moofExpectedMagic, moofMagic);
		fragmentStream.close();

		return {};
	}

	std::string fragmentData;

	fragmentStream.seekg(-8, std::ios::cur);

	auto moof = new char[moofSize];
	fragmentStream.read(moof, moofSize);
	fragmentData.append(moof, moofSize);
	delete[] moof;

	unsigned int mdatSize;
	fragmentStream.read(reinterpret_cast<char*>(&mdatSize), 4);
	mdatSize = _byteswap_ulong(mdatSize);

	std::string mdatExpectedMagic = BLOCK_MDAT;
	std::string mdatMagic(mdatExpectedMagic.length(), '\0');
	fragmentStream.read(mdatMagic.data(), mdatExpectedMagic.length());

	if (mdatMagic != mdatExpectedMagic)
	{
		logger.warning(
			"Invalid mdat magic in fragment at start time %s in track %s, expected: %s, got %s. Will need to fetch that fragment from server.",
			start_time, media.source_file, mdatExpectedMagic, mdatMagic);
		fragmentStream.close();

		return "";
	}

	fragmentStream.seekg(-8, std::ios::cur);

	auto mdat = new char[mdatSize];
	fragmentStream.read(mdat, mdatSize);
	fragmentData.append(mdat, mdatSize);

	delete[] mdat;
	fragmentStream.close();

	return fragmentData;
}
