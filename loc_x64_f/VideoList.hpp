#pragma once

static constexpr unsigned char RMDJEncryptionKey[] =
{
	0xba, 0x7a, 0xbb, 0x27, 0x03, 0x9b, 0x72, 0xfd,
	0x13, 0xeb, 0x70, 0x38, 0x7e, 0x0f, 0xcb, 0x41,
	0xe1, 0xd0, 0xeb, 0x54, 0xbe, 0x8f, 0x13, 0x6d,
	0xf0, 0xba, 0xe2, 0x2a, 0xdc, 0xfb, 0x40, 0xf1
};

class VideoList : public Poco::Util::Subsystem
{
public:
	const char* name() const override;

	std::vector<std::string> getEpisodeList();
	std::string getManifestUrl(std::string episodeId);
	std::string getFragmentUrl(std::string episodeId, std::string bitrate, std::string type, std::string startTime);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	Poco::JSON::Object::Ptr videoList;
};
