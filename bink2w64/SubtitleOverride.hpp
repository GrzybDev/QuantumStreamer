#pragma once

class SubtitleOverride
{
public:
	SubtitleOverride();

	static SubtitleOverride& GetInstance()
	{
		static SubtitleOverride* instance;

		if (instance == nullptr)
			instance = new SubtitleOverride();

		return *instance;
	}

	void LoadOverrides(std::string episode);
	std::string GetSubtitleOverride(std::string episode, std::string subtitleName, std::string& originalChunk);

private:
	bool enableCC = false;
	std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> subtitleOverrides;
};
