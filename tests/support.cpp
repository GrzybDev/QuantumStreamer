#include "support.hpp"

namespace qs::test
{
	std::string bigEndian(const std::uint64_t value, const std::size_t size)
	{
		std::string bytes;
		for (std::size_t i = size; i-- > 0;)
			bytes += static_cast<char>(value >> (i * 8) & 0xFF);
		return bytes;
	}

	std::string box(const std::string_view type, const std::string_view body)
	{
		return bigEndian(8 + body.size()) + std::string(type) + std::string(body);
	}

	std::string fragment(const std::string_view payload, const std::uint32_t sampleCount)
	{
		// Flags 0x701 like the CDN's caption fragments: data offset, then a duration, size and flags per sample.
		std::string samples;
		for (std::uint32_t i = 0; i < sampleCount; ++i)
			samples += bigEndian(20020000) + bigEndian(payload.size() / sampleCount) + bigEndian(0);

		const std::string trun = box("trun", bigEndian(0x701) + bigEndian(sampleCount) + bigEndian(0) + samples);
		const std::string traf = box("traf", box("tfhd", bigEndian(0) + bigEndian(1)) + trun);
		return box("moof", box("mfhd", bigEndian(0) + bigEndian(1)) + traf) + box("mdat", payload);
	}

	std::string trackFile(const std::vector<std::pair<std::uint64_t, std::string>>& fragments, const int version,
	                      const std::uint32_t numberSizes)
	{
		const std::size_t timeSize = version == 1 ? 8 : 4;
		const std::size_t numbersSize = (numberSizes >> 4 & 3) + (numberSizes >> 2 & 3) + (numberSizes & 3) + 3;

		std::string file = box("ftyp", "isml");
		std::string entries;
		for (const auto& [time, bytes] : fragments)
		{
			entries += bigEndian(time, timeSize) + bigEndian(file.size(), timeSize) + std::string(numbersSize, '\1');
			file += bytes;
		}

		const std::string tfra = box("tfra", bigEndian(static_cast<std::uint64_t>(version) << 24) + bigEndian(1)
		                             + bigEndian(numberSizes) + bigEndian(fragments.size()) + entries);
		const std::size_t mfraSize = 8 + tfra.size() + 16;
		return file + box("mfra", tfra + box("mfro", bigEndian(0) + bigEndian(mfraSize)));
	}
}
