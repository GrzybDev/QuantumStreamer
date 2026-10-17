#include <vector>

#include <gtest/gtest.h>
#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/NamespaceSupport.h>

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

		struct Paragraph
		{
			std::string id;
			std::string begin;
			std::string end;
			std::string text;

			bool operator==(const Paragraph&) const = default;
		};

		// Parsing with namespaces on also proves the document is namespace-well-formed.
		std::vector<Paragraph> paragraphs(const std::string& ttml)
		{
			Poco::XML::DOMParser parser;
			const Poco::AutoPtr<Poco::XML::Document> document = parser.parseString(ttml);
			const Poco::AutoPtr<Poco::XML::NodeList> nodes = document->getElementsByTagName("p");

			std::vector<Paragraph> result;
			for (unsigned long i = 0; i < nodes->length(); ++i)
			{
				const auto* p = static_cast<Poco::XML::Element*>(nodes->item(i));
				result.push_back({p->getAttributeNS(Poco::XML::NamespaceSupport::XML_NAMESPACE, "id"),
				                  p->getAttribute("begin"), p->getAttribute("end"), p->innerText()});
			}
			return result;
		}
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

	TEST_F(SubtitleOverridesTest, ReplacesAFragmentsCaptionsWithTheCuesOverlappingIt)
	{
		const SubtitleOverrides overrides = load({.closedCaptioning = true, .musicNotes = true});

		// The fragment's own paragraphs end within a second, so the 2.5 s minimum window applies.
		EXPECT_EQ(paragraphs(overrides.apply("E1", "enus_captions", kTenSeconds, kFragment)),
		          (std::vector<Paragraph>{
			          {"p1", "00:00:00.500", "00:00:02.000", "- [ door slams ] Who is there? [ gasps ]"},
			          {"p2", "00:00:02.200", "00:00:04.000", "\xE2\x99\xAA\xE2\x99\xAA Humming"},
			          }));
	}

	TEST_F(SubtitleOverridesTest, KeepsReadingCuesAfterOneWithBrokenTiming)
	{
		const SubtitleOverrides overrides = load({.closedCaptioning = true, .musicNotes = true});

		EXPECT_EQ(paragraphs(overrides.apply("E1", "enus_captions", 3 * kTenSeconds, kFragment)),
		          (std::vector<Paragraph>{{"p1", "00:00:00.000", "00:00:01.000", "First line\nSecond line"}}));
	}

	TEST_F(SubtitleOverridesTest, DropsAnnotationsButKeepsTheDialogueBetweenThem)
	{
		const SubtitleOverrides overrides = load({.closedCaptioning = false, .musicNotes = false});

		const std::vector<Paragraph> result = paragraphs(overrides.apply("E1", "enus_captions", kTenSeconds, kFragment));

		ASSERT_EQ(result.size(), 2u);
		EXPECT_EQ(result[0].text, " Who is there?");
		EXPECT_EQ(result[1].text, " Humming");
	}

	TEST_F(SubtitleOverridesTest, LeavesTracksWithoutAnOverrideUntouched)
	{
		const SubtitleOverrides overrides = load({});

		EXPECT_TRUE(overrides.overrides("E1", "enus_captions"));
		EXPECT_FALSE(overrides.overrides("E1", "frfr_captions"));
		EXPECT_FALSE(overrides.overrides("E2", "enus_captions"));
		EXPECT_EQ(overrides.apply("E1", "frfr_captions", kTenSeconds, kFragment), kFragment);
	}
}
