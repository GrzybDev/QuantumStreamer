#include "pch.hpp"

SmoothStreaming::SmoothStreaming()
{
	BOOST_LOG_FUNCTION();
	BOOST_LOG_TRIVIAL(info) << "Preloading smooth streams...";

	// Read all .ism files in episodes directory
	if (exists(episodes_path) && is_directory(episodes_path))
	{
		for (const auto& episode_dir : boost::filesystem::directory_iterator(episodes_path))
		{
			// Find all .ism files inside the episode directory

			if (is_directory(episode_dir))
			{
				for (const auto& entry : boost::filesystem::directory_iterator(episode_dir))
				{
					if (is_regular_file(entry) && entry.path().extension().string() == ".ism")
					{
						BOOST_LOG_TRIVIAL(debug) << "Found smooth stream: " << entry.path().string();

						std::string episode_id = entry.path().parent_path().filename().string();

						pugi::xml_document serverManifest;
						pugi::xml_parse_result loadSuccess = serverManifest.load_file(entry.path().string().c_str());

						if (loadSuccess)
						{
							auto clientRelativePath = serverManifest.select_node(
								"//head/meta[@name='clientManifestRelativePath']");
							auto clientManifestPath = clientRelativePath.node().attribute("content").value();

							clientManifestPaths[episode_id] = clientManifestPath;

							auto videoNodes = serverManifest.select_nodes("//video");
							auto audioNodes = serverManifest.select_nodes("//audio");
							auto textNodes = serverManifest.select_nodes("//textstream");

							for (const auto& videoNode : videoNodes)
							{
								std::string streamPath = videoNode.node().attribute("src").value();

								if (exists(episode_dir / streamPath))
								{
									SmoothMedia media{};
									media.type = Video;
									media.name = "video";
									media.src = streamPath;
									media.systemBitrate =
										std::stoi(videoNode.node().attribute("systemBitrate").value());
									auto stream = PreloadStream((episode_dir / streamPath).string());

									if (stream.success)
									{
										media.stream = stream;
										smoothStreams[episode_id].push_back(media);
										BOOST_LOG_TRIVIAL(debug) << "Preloaded video stream for " << episode_id << ":" << streamPath;
									}
									else
									{
										BOOST_LOG_TRIVIAL(error) << "Failed to preload video stream: " << streamPath;
									}
								}
							}

							for (const auto& audioNode : audioNodes)
							{
								auto node = audioNode.node();

								std::string streamPath = node.attribute("src").value();
								std::string trackName = node.select_node("param[@name='trackName']").node().attribute(
									"value").value();

								if (exists(episode_dir / streamPath))
								{
									SmoothMedia media{};
									media.type = Audio;
									media.name = trackName;
									media.src = streamPath;
									media.systemBitrate =
										std::stoi(audioNode.node().attribute("systemBitrate").value());
									auto stream = PreloadStream((episode_dir / streamPath).string());

									if (stream.success)
									{
										media.stream = stream;
										smoothStreams[episode_id].push_back(media);
										BOOST_LOG_TRIVIAL(debug) << "Preloaded audio stream for " << episode_id << ":" << streamPath;
									}
									else
									{
										BOOST_LOG_TRIVIAL(error) << "Failed to preload audio stream: " << streamPath;
									}
								}
							}

							for (const auto& textNode : textNodes)
							{
								auto node = textNode.node();
								std::string streamPath = node.attribute("src").value();
								std::string trackName = node.select_node("param[@name='trackName']").node().attribute(
									"value").value();

								if (exists(episode_dir / streamPath))
								{
									SmoothMedia media{};
									media.type = Text;
									media.name = trackName;
									media.src = streamPath;
									media.systemBitrate = std::stoi(textNode.node().attribute("systemBitrate").value());
									auto stream = PreloadStream((episode_dir / streamPath).string());
									if (stream.success)
									{
										media.stream = stream;
										smoothStreams[episode_id].push_back(media);
										BOOST_LOG_TRIVIAL(debug) << "Preloaded text stream for " << episode_id << ":" << streamPath;
									}
									else
									{
										BOOST_LOG_TRIVIAL(error) << "Failed to preload text stream: " << streamPath;
									}
								}
							}
						}

						break;
					}
				}
			}
		}
	}

	BOOST_LOG_TRIVIAL(info) << "Finished preloading smooth streams!";
}

SmoothStreaming::SmoothStream SmoothStreaming::PreloadStream(std::string filePath)
{
	SmoothStream stream;

	std::ifstream fileStream(filePath, std::ios::binary);
	if (!fileStream.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open smooth stream file: " << filePath;
		return stream;
	}

	// Read mfro box
	fileStream.seekg(-4, std::ios::end);

	int mfroSize;
	fileStream.read(reinterpret_cast<char*>(&mfroSize), 4);
	mfroSize = _byteswap_ulong(mfroSize);

	fileStream.seekg(-mfroSize, std::ios::end);

	unsigned int mfraBlockSize;

	fileStream.read(reinterpret_cast<char*>(&mfraBlockSize), 4);

	mfraBlockSize = _byteswap_ulong(mfraBlockSize);

	if (mfraBlockSize != mfroSize)
	{
		BOOST_LOG_TRIVIAL(error) << "Invalid mfro block size, expected: " << mfroSize << ", got: " << mfraBlockSize;
		return stream;
	}

	std::string mfraMagic(4, '\0');
	fileStream.read(&mfraMagic[0], 4);

	if (mfraMagic != "mfra")
	{
		BOOST_LOG_TRIVIAL(error) << "Invalid mfra magic, expected: mfra, got: " << mfraMagic;
		return stream;
	}

	unsigned int tfraSize;
	fileStream.read(reinterpret_cast<char*>(&tfraSize), 4);
	tfraSize = _byteswap_ulong(tfraSize);

	std::string tfraMagic(4, '\0');
	fileStream.read(&tfraMagic[0], 4);

	if (tfraMagic != "tfra")
	{
		BOOST_LOG_TRIVIAL(error) << "Invalid tfra magic, expected: tfra, got: " << tfraMagic;
		return stream;
	}

	char version;
	fileStream.read(&version, 1);
	// Skip flags
	fileStream.seekg(3, std::ios::cur);

	unsigned int trackId;
	fileStream.read(reinterpret_cast<char*>(&trackId), 4);
	stream.trackId = _byteswap_ulong(trackId);

	int temp;
	fileStream.read(reinterpret_cast<char*>(&temp), 4);
	stream.lengthSizeOfTrafNum = ((temp & 0x3F) >> 4) + 1;
	stream.lengthSizeOfTrunNum = ((temp & 0xC) >> 2) + 1;
	stream.lengthSizeOfSampleNum = ((temp & 0x3)) + 1;

	unsigned int numberOfEntries;
	fileStream.read(reinterpret_cast<char*>(&numberOfEntries), 4);
	numberOfEntries = _byteswap_ulong(numberOfEntries);

	std::list<SmoothFragment> fragments;

	for (unsigned int i = 0; i < numberOfEntries; i++)
	{
		SmoothFragment fragment{};

		if (version == 1)
		{
			long long time;
			fileStream.read(reinterpret_cast<char*>(&time), 8);
			fragment.time = _byteswap_uint64(time);

			long long moofOffset;
			fileStream.read(reinterpret_cast<char*>(&moofOffset), 8);
			fragment.moofOffset = _byteswap_uint64(moofOffset);
		}
		else
		{
			unsigned int time;
			fileStream.read(reinterpret_cast<char*>(&time), 4);
			fragment.time = _byteswap_ulong(time);
			unsigned int moofOffset;
			fileStream.read(reinterpret_cast<char*>(&moofOffset), 4);
			fragment.moofOffset = _byteswap_ulong(moofOffset);
		}

		long long trafNumber;
		fileStream.read(reinterpret_cast<char*>(&trafNumber), stream.lengthSizeOfTrafNum);
		fragment.trafNumber = _byteswap_uint64(trafNumber);
		long long trunNumber;
		fileStream.read(reinterpret_cast<char*>(&trunNumber), stream.lengthSizeOfTrunNum);
		fragment.trunNumber = _byteswap_uint64(trunNumber);
		long long sampleNumber;
		fileStream.read(reinterpret_cast<char*>(&sampleNumber), stream.lengthSizeOfSampleNum);
		fragment.sampleNumber = _byteswap_uint64(sampleNumber);
		fragments.push_back(fragment);
	}

	stream.fragments = fragments;
	stream.success = true;
	return stream;
}

std::string SmoothStreaming::GetClientManifest(std::string episodeId)
{
	if (clientManifestPaths.find(episodeId) == clientManifestPaths.end())
	{
		BOOST_LOG_TRIVIAL(warning) << "Client manifest for episode " << episodeId << " not found.";
		return "";
	}

	std::string clientManifestPath = (episodes_path / episodeId / clientManifestPaths[episodeId]).string();
	std::ifstream fileStream(clientManifestPath, std::ios::binary);

	if (!fileStream.is_open())
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open client manifest file: " << clientManifestPath;
		return "";
	}

	BOOST_LOG_TRIVIAL(debug) << "Found local client manifest for episode: " << episodeId;

	std::vector<char> fileBytes((std::istreambuf_iterator<char>(fileStream)),
	                            std::istreambuf_iterator<char>());

	return std::string(fileBytes.begin(), fileBytes.end());
}

std::string SmoothStreaming::GetFragment(std::string episodeId, std::string trackName, long long bitrate,
                                         long long time)
{
	if (smoothStreams.find(episodeId) == smoothStreams.end())
		return "";

	for (const auto& media : smoothStreams[episodeId])
	{
		if (media.name == trackName && media.systemBitrate == bitrate)
		{
			for (const auto& fragment : media.stream.fragments)
			{
				if (fragment.time == time)
				{
					std::string fragmentData;

					std::ifstream fileStream((episodes_path / episodeId / media.src).string(), std::ios::binary);

					if (!fileStream.is_open())
					{
						BOOST_LOG_TRIVIAL(error) << "Failed to open fragment file: " << media.src;
						return "";
					}

					BOOST_LOG_TRIVIAL(debug) << "Found local fragment at time: " << time << " for " << episodeId << ":" << trackName << " in " << media.src;

					fileStream.seekg(fragment.moofOffset);

					unsigned int moofSize;
					fileStream.read(reinterpret_cast<char*>(&moofSize), 4);
					moofSize = _byteswap_ulong(moofSize);
					fileStream.seekg(-4, std::ios::cur);

					auto moof = new char[moofSize];
					fileStream.read(moof, moofSize);
					fragmentData.append(moof, moofSize);
					delete[] moof;

					unsigned int mdatSize;
					fileStream.read(reinterpret_cast<char*>(&mdatSize), 4);
					mdatSize = _byteswap_ulong(mdatSize);
					fileStream.seekg(-4, std::ios::cur);

					auto mdat = new char[mdatSize];
					fileStream.read(mdat, mdatSize);

					if (media.type == Text)
					{
						// Skip first 8 bytes and read it as a string
						std::string mdatString(mdat + 8, mdatSize - 8);

						SubtitleOverride& subtitleOverride = SubtitleOverride::GetInstance();
						std::string newString = subtitleOverride.GetSubtitleOverride(episodeId, trackName, mdatString);

						// Update the mdat size
						mdatSize = newString.size() + 8;

						// Update the mdat
						delete[] mdat;
						mdat = new char[mdatSize];

						// Write mdatSize in big-endian
						unsigned int mdatSizeBE = _byteswap_ulong(mdatSize);
						memcpy(mdat, &mdatSizeBE, 4);
						memcpy(mdat + 4, "mdat", 4);
						memcpy(mdat + 8, newString.c_str(), newString.size());
					}

					fragmentData.append(mdat, mdatSize);

					delete[] mdat;

					fileStream.close();
					return fragmentData;
				}
			}
		}
	}

	return "";
}
