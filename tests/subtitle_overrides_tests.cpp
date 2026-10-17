#include <gtest/gtest.h>

#include "subtitles/subtitle_overrides.hpp"
#include "support.hpp"

namespace qs::test
{
	namespace
	{
		constexpr std::uint64_t kTenSeconds = 100'000'000;

		constexpr auto kFragment =
			R"(<?xml version="1.0" encoding="UTF-8"?><tt xml:lang="eng" xmlns="http://www.w3.org/ns/ttml"><head/>)"
			R"(<body><div><p xml:id="s1" begin="00:00:00.000" end="00:00:01.000" region="speaker">)"
			R"(<span style="textStyle">ORIGINAL</span></p></div></body></tt>)";
	}

	class SubtitleOverridesTest : public testing::Test
	{
	protected:
		SubtitleOverridesTest()
		{
			folder_.write("E1/enus_captions.srt",
			              "\xEF\xBB\xBF" "1\r\n00:00:10,500 --> 00:00:12,000\r\n"
			              "- [ door slams ] Who is there? [ gasps ]\r\n\r\n"
			              "2\r\n00:00:12,200 --> 00:00:14,000\r\n\xE2\x99\xAA\xE2\x99\xAA Humming\r\n\r\n"
			              "3\r\nbroken --> timing\r\nNever shown\r\n\r\n"
			              "4\r\n00:00:30,000 --> 00:00:31,000\r\nFirst line\r\nSecond line\r\n");
		}

		[[nodiscard]] SubtitleOverrides load(const SubtitleOptions options) const
		{
			return {folder_.path().string(), {"E1", "E2"}, options};
		}

		TempFolder folder_;
	};

	TEST_F(SubtitleOverridesTest, LeavesTracksWithoutAnOverrideUntouched)
	{
		const SubtitleOverrides overrides = load({});

		EXPECT_TRUE(overrides.overrides("E1", "enus_captions"));
		EXPECT_FALSE(overrides.overrides("E1", "frfr_captions"));
		EXPECT_FALSE(overrides.overrides("E2", "enus_captions"));
		EXPECT_EQ(overrides.apply("E1", "frfr_captions", kTenSeconds, kFragment), kFragment);
	}
}
