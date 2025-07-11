#pragma once

static constexpr unsigned char RMDJ_ENCRYPTION_KEY[] =
{
	0xba, 0x7a, 0xbb, 0x27, 0x03, 0x9b, 0x72, 0xfd, 0x13, 0xeb, 0x70, 0x38, 0x7e, 0x0f, 0xcb, 0x41,
	0xe1, 0xd0, 0xeb, 0x54, 0xbe, 0x8f, 0x13, 0x6d, 0xf0, 0xba, 0xe2, 0x2a, 0xdc, 0xfb, 0x40, 0xf1
};

class VideoList final : public Poco::Util::Subsystem
{
public:
	[[nodiscard]] const char* name() const override;

	std::string getManifestUrl(const std::string& episode_id);
	std::string getFragmentUrl(const std::string& episode_id, const std::string& bitrate, const std::string& type,
	                           const std::string& start_time);

	std::vector<std::string> getEpisodeList();
	void patch(unsigned short port);

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	Poco::JSON::Object::Ptr video_list_;

	Poco::JSON::Object::Ptr loadVideoList(const std::string& path) const;
};
