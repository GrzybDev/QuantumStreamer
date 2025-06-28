#pragma once

class OfflineStreaming final : public Poco::Util::Subsystem
{
public:
	[[nodiscard]] const char* name() const override;

protected:
	void initialize(Poco::Util::Application& app) override;
	void uninitialize() override;

private:
	struct SmoothFragment
	{
		unsigned long long moof_offset;
		unsigned long long traf_number;
		unsigned long long trun_number;
		unsigned long long sample_number;
	};

	struct SmoothTrack
	{
		char version;
		unsigned int track_id;
		int length_size_of_traf_num;
		int length_size_of_trun_num;
		int length_size_of_sample_num;
		std::map<std::string, SmoothFragment> fragments;
	};

	struct SmoothMedia
	{
		Poco::Path source_file;
		std::string system_bitrate;
		SmoothTrack track;
	};

	struct SmoothStream
	{
		Poco::Path client_manifest_relative_path;
		std::map<std::string, SmoothMedia> media_map;
	};

	std::map<std::string, SmoothStream> streams_;

	void processMediaNodes(const std::string& tag_name, Poco::XML::Document* doc, const std::string& episode_id,
	                       const std::string& episode_path, SmoothStream& stream) const;
	[[nodiscard]] std::pair<bool, SmoothTrack> preloadTrack(const std::string& path) const;
};
