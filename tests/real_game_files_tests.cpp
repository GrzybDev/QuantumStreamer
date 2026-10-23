#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <Poco/Environment.h>

#include "catalog/video_list.hpp"
#include "subtitles/subtitle_overrides.hpp"
#include "support.hpp"

namespace qs::test
{
	namespace
	{
		// These checks read a real game install, so they run only when this variable names its folder.
		constexpr auto kGameFolderVariable = "QUANTUMSTREAMER_GAME_DIR";

		// Its only paragraph lasts ten hours, so every cue of an override overlaps the fragment.
		constexpr auto kWholeEpisodeCaptions =
			R"(<?xml version="1.0" encoding="UTF-8"?><tt xmlns="http://www.w3.org/ns/ttml"><body><div>)"
			R"(<p xml:id="s1" begin="00:00:00.000" end="10:00:00.000"><span>ORIGINAL</span></p></div></body></tt>)";

		std::filesystem::path gameFolder()
		{
			return Poco::Environment::get(kGameFolderVariable, "");
		}
	}

	TEST(RealGameFiles, DecryptsTheGamesVideoLists)
	{
		const std::filesystem::path game = gameFolder();
		if (game.empty())
			GTEST_SKIP() << kGameFolderVariable << " is not set";

		std::vector<std::filesystem::path> lists;
		for (const char* name : {"data/videoList.rmdj", "data/videoList_original.rmdj"})
			if (std::filesystem::exists(game / name))
				lists.push_back(game / name);

		ASSERT_FALSE(lists.empty());
		for (const std::filesystem::path& list : lists)
			EXPECT_FALSE(VideoList::load(list.string()).episodeIds().empty()) << list;
	}

	TEST(RealGameFiles, AppliesEveryStoredCaptionOverride)
	{
		const std::filesystem::path game = gameFolder();
		if (game.empty())
			GTEST_SKIP() << kGameFolderVariable << " is not set";

		const std::filesystem::path episodes = game / "videos" / "episodes";
		if (!std::filesystem::is_directory(episodes))
			GTEST_SKIP() << "No episodes are stored";

		std::vector<std::string> episodeIds;
		std::vector<std::pair<std::string, std::string>> tracks;
		for (const auto& folder : std::filesystem::directory_iterator(episodes))
		{
			if (!folder.is_directory())
				continue;

			episodeIds.push_back(folder.path().filename().string());
			for (const auto& file : std::filesystem::directory_iterator(folder))
				if (const std::string track = file.path().stem().string();
					file.path().extension() == ".srt" && track.find("_captions") != std::string::npos)
					tracks.emplace_back(episodeIds.back(), track);
		}

		if (tracks.empty())
			GTEST_SKIP() << "No caption overrides are stored";

		const SubtitleOverrides overrides(episodes.string(), episodeIds, {});
		for (const auto& [episodeId, track] : tracks)
			EXPECT_FALSE(paragraphs(overrides.apply(episodeId, track, 0, kWholeEpisodeCaptions)).empty())
				<< episodeId << "/" << track;
	}
}
