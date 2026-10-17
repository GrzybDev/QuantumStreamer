#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace qs::test
{
	/// Builds an MP4 box.
	/// @param type The four-character box type.
	/// @param body Everything after the box header.
	std::string box(std::string_view type, std::string_view body);

	/// Encodes an integer as big-endian bytes.
	/// @param value The integer to encode.
	/// @param size How many bytes to write.
	std::string bigEndian(std::uint64_t value, std::size_t size = 4);

	/// Builds a moof + mdat fragment whose trun box lists the sizes of its samples.
	/// @param payload The content of the mdat box.
	/// @param sampleCount How many samples share the payload.
	std::string fragment(std::string_view payload, std::uint32_t sampleCount = 1);

	/// Builds a track file of fragments followed by its mfra index, as QuantumFetcher writes it.
	/// @param fragments Each fragment's start time and bytes.
	/// @param version The tfra version, 1 for 64-bit times and offsets.
	/// @param numberSizes The tfra field giving the sizes of the traf, trun and sample numbers.
	std::string trackFile(const std::vector<std::pair<std::uint64_t, std::string>>& fragments, int version = 1,
	                      std::uint32_t numberSizes = 0);
}
