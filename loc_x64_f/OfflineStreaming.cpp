#include "pch.hpp"
#include "OfflineStreaming.hpp"

#include "VideoList.hpp"

using Poco::AutoPtr;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Logger;
using Poco::Path;
using Poco::Util::Application;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::InputSource;
using Poco::XML::Node;
using Poco::XML::NodeList;
using XMLDocument = Poco::XML::Document;

const char* OfflineStreaming::name() const
{
	return "OfflineStreaming";
}

void OfflineStreaming::initialize(Application& app)
{
	Logger& logger = Logger::get("Server");
	logger.information("Initializing offline playback module...");

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
			stream.clientManifestRelativePath = clientManifestPath;

			processMediaNodes("video", doc, episode, episodePath.toString(), stream);
			processMediaNodes("audio", doc, episode, episodePath.toString(), stream);
			processMediaNodes("textstream", doc, episode, episodePath.toString(), stream);

			_streams[episode] = stream;
		}
	}
}

void OfflineStreaming::uninitialize()
{
	_streams.clear();
}

void OfflineStreaming::processMediaNodes(const std::string& tagName, XMLDocument* doc, const std::string& episodeId,
                                         const std::string& episodePath, SmoothStream& stream)
{
	Logger& logger = Logger::get("Server");

	NodeList* nodes = doc->getElementsByTagName(tagName);
	for (unsigned long i = 0; i < nodes->length(); ++i)
	{
		auto* elem = dynamic_cast<Element*>(nodes->item(i));
		std::string src = elem->getAttribute("src");
		std::string bitrate = elem->getAttribute("systemBitrate");

		std::string trackName = "video";
		if (tagName != "video")
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

		Path fullPath = episodePath;
		fullPath.append(src);
		if (!fullPath.isFile())
			continue;

		SmoothMedia media;
		media.sourceFile = fullPath;
		media.systemBitrate = bitrate;

		if (auto [success, track] = preloadTrack(fullPath.toString()); success)
		{
			media.track[bitrate] = track;
			stream.mediaMap[trackName] = media;
			logger.debug("Preloaded %s track '%s' for episode %s from %s with bitrate %s", tagName, trackName,
			             episodeId, fullPath.toString(), bitrate);
		}
	}
}

std::pair<bool, OfflineStreaming::SmoothTrack> OfflineStreaming::preloadTrack(const std::string& path)
{
	Logger& logger = Logger::get("Server");

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
	trackStream.read(reinterpret_cast<char*>(&mfroSize), 4);
	mfroSize = _byteswap_ulong(mfroSize);

	trackStream.seekg(-static_cast<INT>(mfroSize), std::ios::end);

	// Read mfra box
	unsigned int mfraBlockSize;
	trackStream.read(reinterpret_cast<char*>(&mfraBlockSize), 4);
	mfraBlockSize = _byteswap_ulong(mfraBlockSize);

	if (mfraBlockSize != mfroSize)
	{
		logger.warning("Invalid mfro block size in track file %s, expected %s, got: %s, skipping this track.", path,
		               std::to_string(mfroSize), std::to_string(mfraBlockSize));
		trackStream.close();

		return {success, track};
	}

	std::string mfraMagic(4, '\0');
	trackStream.read(mfraMagic.data(), 4);

	if (mfraMagic != "mfra")
	{
		logger.warning("Invalid mfra magic in track file %s, expected: mfra, got: %s, skipping this track.", path,
		               mfraMagic);
		trackStream.close();

		return {success, track};
	}

	// Read tfra box
	unsigned int tfraSize;
	trackStream.read(reinterpret_cast<char*>(&tfraSize), 4);
	tfraSize = _byteswap_ulong(tfraSize);

	std::string tfraMagic(4, '\0');
	trackStream.read(tfraMagic.data(), 4);

	if (tfraMagic != "tfra")
	{
		logger.warning("Invalid tfra magic in track file %s, expected: tfra, got: %s, skipping this track.", path,
		               tfraMagic);
		trackStream.close();

		return {success, track};
	}

	char version;
	trackStream.read(&version, 1);
	// Skip flags
	trackStream.seekg(3, std::ios::cur);

	unsigned int trackId;
	trackStream.read(reinterpret_cast<char*>(&trackId), 4);
	track.trackId = _byteswap_ulong(trackId);

	int temp;
	trackStream.read(reinterpret_cast<char*>(&temp), 4);
	track.lengthSizeOfTrafNum = ((temp & 0x3F) >> 4) + 1;
	track.lengthSizeOfTrunNum = ((temp & 0xC) >> 2) + 1;
	track.lengthSizeOfSampleNum = ((temp & 0x3)) + 1;

	unsigned int numberOfEntries;
	trackStream.read(reinterpret_cast<char*>(&numberOfEntries), 4);
	numberOfEntries = _byteswap_ulong(numberOfEntries);

	std::map<std::string, SmoothFragment> fragments;

	for (unsigned int i = 0; i < numberOfEntries; i++)
	{
		SmoothFragment fragment{};

		unsigned long long startTime;

		if (version == 1)
		{
			unsigned long long time;
			trackStream.read(reinterpret_cast<char*>(&time), 8);
			startTime = _byteswap_uint64(time);

			unsigned long long moofOffset;
			trackStream.read(reinterpret_cast<char*>(&moofOffset), 8);
			fragment.moofOffset = _byteswap_uint64(moofOffset);
		}
		else
		{
			unsigned int time;
			trackStream.read(reinterpret_cast<char*>(&time), 4);
			startTime = _byteswap_ulong(time);

			unsigned int moofOffset;
			trackStream.read(reinterpret_cast<char*>(&moofOffset), 4);
			fragment.moofOffset = _byteswap_ulong(moofOffset);
		}

		unsigned long long trafNumber;
		trackStream.read(reinterpret_cast<char*>(&trafNumber), track.lengthSizeOfTrafNum);
		fragment.trafNumber = _byteswap_uint64(trafNumber);

		unsigned long long trunNumber;
		trackStream.read(reinterpret_cast<char*>(&trunNumber), track.lengthSizeOfTrunNum);
		fragment.trunNumber = _byteswap_uint64(trunNumber);

		unsigned long long sampleNumber;
		trackStream.read(reinterpret_cast<char*>(&sampleNumber), track.lengthSizeOfSampleNum);
		fragment.sampleNumber = _byteswap_uint64(sampleNumber);

		fragments[std::to_string(startTime)] = fragment;
	}

	track.fragments = fragments;
	success = true;
	trackStream.close();

	return {success, track};
}

std::string OfflineStreaming::GetLocalClientManifest(const std::string& episodeId)
{
	if (!_streams.contains(episodeId))
		return "";

	Logger& logger = Logger::get("Server");

	Path clientManifestRelativePath = _streams[episodeId].clientManifestRelativePath;
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

std::string OfflineStreaming::GetLocalFragment(const std::string& episodeId, const std::string& trackName,
                                               const std::string& bitrate,
                                               const std::string& startTime)
{
	if (!_streams.contains(episodeId))
		return {};

	Logger& logger = Logger::get("Server");

	auto [clientManifestRelativePath, mediaMap] = _streams[episodeId];

	if (!mediaMap.contains(trackName))
		return {};

	SmoothMedia media = mediaMap[trackName];

	if (!media.track.contains(bitrate))
		return {};

	SmoothTrack track = media.track[bitrate];

	if (!track.fragments.contains(startTime))
		return {};

	SmoothFragment fragment = track.fragments[startTime];

	std::ifstream fragmentStream(media.sourceFile.toString(), std::ios::binary);

	if (!fragmentStream)
	{
		logger.warning(
			"Failed to open track file %s, the file was there while initializing, but it probably got deleted. Will need to fetch client manifest from server.",
			media.sourceFile.toString());
		fragmentStream.close();

		return {};
	}

	fragmentStream.seekg(static_cast<long long>(fragment.moofOffset));

	unsigned int moofSize;
	fragmentStream.read(reinterpret_cast<char*>(&moofSize), 4);
	moofSize = _byteswap_ulong(moofSize);

	std::string moofMagic(4, '\0');
	fragmentStream.read(moofMagic.data(), 4);

	if (moofMagic != "moof")
	{
		logger.warning(
			"Invalid moof magic in fragment at start time %s in track %s, expected: moof, got %s. Will need to fetch that fragment from server.",
			startTime, media.sourceFile, moofMagic);
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

	std::string mdatMagic(4, '\0');
	fragmentStream.read(mdatMagic.data(), 4);

	if (mdatMagic != "mdat")
	{
		logger.warning(
			"Invalid mdat magic in fragment at start time %s in track %s, expected: mdat, got %s. Will need to fetch that fragment from server.",
			startTime, media.sourceFile, mdatMagic);
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
