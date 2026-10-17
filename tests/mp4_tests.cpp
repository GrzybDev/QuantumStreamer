#include <sstream>

#include <gtest/gtest.h>

#include "media/mp4.hpp"
#include "support.hpp"

namespace qs::test
{
	namespace
	{
		std::istringstream stream(const std::string& bytes)
		{
			return std::istringstream(bytes, std::ios::binary);
		}
	}

	TEST(Mp4, IndexesTheFragmentsOfATrackFileAsQuantumFetcherWritesIt)
	{
		const std::string first = fragment("first");
		const std::string second = fragment("second");
		auto file = stream(trackFile({{0, first}, {20020000, second}}));

		const mp4::FragmentIndex index = mp4::readFragmentIndex(file);

		ASSERT_EQ(index.size(), 2u);
		EXPECT_EQ(mp4::readFragment(file, index.at(0)), first);
		EXPECT_EQ(mp4::readFragment(file, index.at(20020000)), second);
	}

	TEST(Mp4, ReadsThirtyTwoBitIndexEntriesWithWideNumbers)
	{
		const std::string only = fragment("only");
		// Two-byte traf, three-byte trun and four-byte sample numbers.
		auto file = stream(trackFile({{40040000, only}}, 0, 0b011011));

		const mp4::FragmentIndex index = mp4::readFragmentIndex(file);

		ASSERT_EQ(index.size(), 1u);
		EXPECT_EQ(mp4::readFragment(file, index.at(40040000)), only);
	}

	TEST(Mp4, RejectsAFileWhoseMfroDoesNotMatchItsMfra)
	{
		std::string bytes = trackFile({{0, fragment("x")}});
		bytes.back() = static_cast<char>(bytes.back() + 1);
		auto file = stream(bytes);

		EXPECT_THROW((void)mp4::readFragmentIndex(file), mp4::FormatError);
	}

	TEST(Mp4, RejectsAnIndexCutShort)
	{
		std::string bytes = trackFile({{0, fragment("x")}});
		// The entry count sits before the only 19-byte entry and the 16-byte mfro box; it now claims two entries.
		bytes.replace(bytes.size() - 16 - 19 - 4, 4, bigEndian(2));
		auto file = stream(bytes);

		EXPECT_THROW((void)mp4::readFragmentIndex(file), mp4::FormatError);
	}

	TEST(Mp4, RejectsAFragmentRunningPastTheEndOfTheFile)
	{
		const std::string whole = fragment("payload");
		auto file = stream(whole.substr(0, whole.size() - 3));

		EXPECT_THROW((void)mp4::readFragment(file, 0), mp4::FormatError);
	}
}
