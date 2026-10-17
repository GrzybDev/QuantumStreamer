#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace qs::mp4
{
	/// Thrown when a file or fragment does not have the expected box layout.
	class FormatError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	/// Fragment start times mapped to the file offset of their moof box.
	using FragmentIndex = std::map<std::uint64_t, std::uint64_t>;

	/// The moof box of a fragment and the payload of the mdat box after it.
	struct FragmentParts
	{
		std::string_view moof;
		std::string_view payload;
	};

	/// Reads the mfra/tfra index at the end of a fragmented track file.
	/// @param file The track file, opened in binary mode.
	FragmentIndex readFragmentIndex(std::istream& file);

	/// Reads the moof box at an offset together with the mdat box that follows it.
	/// @param file The track file, opened in binary mode.
	/// @param offset Where the moof box starts.
	std::string readFragment(std::istream& file, std::uint64_t offset);

	/// Splits a moof + mdat fragment into its parts.
	/// @param fragment The fragment, which must outlive the returned views.
	FragmentParts splitFragment(std::string_view fragment);

	/// Builds a moof + mdat fragment.
	/// @param moof The fragment's moof box.
	/// @param payload The content of the mdat box.
	std::string joinFragment(std::string_view moof, std::string_view payload);
}
