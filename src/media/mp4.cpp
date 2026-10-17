#include "pch.hpp"
#include "mp4.hpp"

namespace qs::mp4
{
	namespace
	{
		constexpr std::size_t kHeaderSize = 8;
		constexpr std::size_t kSizeFieldSize = 4;

		class ByteReader
		{
		public:
			explicit ByteReader(const std::string_view data) : data_(data)
			{
			}

			std::uint64_t readUInt(const std::size_t bytes)
			{
				std::uint64_t value = 0;
				for (const char byte : take(bytes))
					value = value << 8 | static_cast<unsigned char>(byte);

				return value;
			}

			std::string_view take(const std::size_t bytes)
			{
				if (bytes > data_.size() - position_)
					throw FormatError("Unexpected end of data");

				const std::string_view chunk = data_.substr(position_, bytes);
				position_ += bytes;
				return chunk;
			}

			void skip(const std::size_t bytes)
			{
				take(bytes);
			}

			/// Reads a box header of the given type and returns the size of the whole box.
			std::uint64_t expectBox(const std::string_view type)
			{
				const std::uint64_t size = readUInt(kSizeFieldSize);
				if (const std::string_view actual = take(type.size()); actual != type)
					throw FormatError(std::format("Expected a {} box", type));
				if (size < kHeaderSize)
					throw FormatError(std::format("The {} box is too small", type));

				return size;
			}

		private:
			std::string_view data_;
			std::size_t position_ = 0;
		};

		void appendExactly(std::istream& file, const std::uint64_t size, std::string& out)
		{
			const std::size_t offset = out.size();
			out.resize(offset + static_cast<std::size_t>(size));
			if (!file.read(out.data() + offset, static_cast<std::streamsize>(size)))
				throw FormatError("Unexpected end of file");
		}

		std::string readExactly(std::istream& file, const std::uint64_t size)
		{
			std::string data;
			appendExactly(file, size, data);
			return data;
		}

		std::uint64_t fileSize(std::istream& file)
		{
			file.seekg(0, std::ios::end);
			return static_cast<std::uint64_t>(file.tellg());
		}

		void appendBox(std::istream& file, const std::uint64_t fileEnd, const std::string_view type, std::string& out)
		{
			const auto start = static_cast<std::uint64_t>(file.tellg());
			const std::size_t headerOffset = out.size();
			appendExactly(file, kHeaderSize, out);
			const std::uint64_t size = ByteReader(std::string_view(out).substr(headerOffset)).expectBox(type);
			if (size > fileEnd - start)
				throw FormatError(std::format("The {} box runs past the end of the file", type));

			appendExactly(file, size - kHeaderSize, out);
		}
	}

	FragmentIndex readFragmentIndex(std::istream& file)
	{
		const std::uint64_t size = fileSize(file);
		if (size < kSizeFieldSize)
			throw FormatError("The file is too small");

		// The mfro box closing the file ends with the size of the whole mfra box.
		file.seekg(static_cast<std::streamoff>(size - kSizeFieldSize));
		const std::string mfroTail = readExactly(file, kSizeFieldSize);
		const std::uint64_t mfraSize = ByteReader(mfroTail).readUInt(kSizeFieldSize);
		if (mfraSize < kHeaderSize || mfraSize > size)
			throw FormatError("The file does not end with an mfra box");

		file.seekg(static_cast<std::streamoff>(size - mfraSize));
		const std::string mfra = readExactly(file, mfraSize);
		ByteReader reader(mfra);
		if (reader.expectBox("mfra") != mfraSize)
			throw FormatError("The mfra and mfro sizes differ");

		reader.expectBox("tfra");
		const std::uint64_t version = reader.readUInt(1);
		reader.skip(3 + 4); // flags and track ID
		const std::uint64_t numberSizes = reader.readUInt(4);
		const std::size_t timeSize = version == 1 ? 8 : 4;
		const std::size_t numbersSize = (numberSizes >> 4 & 3) + (numberSizes >> 2 & 3) + (numberSizes & 3) + 3;
		const std::uint64_t entryCount = reader.readUInt(4);

		FragmentIndex index;
		for (std::uint64_t i = 0; i < entryCount; ++i)
		{
			const std::uint64_t time = reader.readUInt(timeSize);
			index[time] = reader.readUInt(timeSize);
			// The traf, trun and sample numbers are not needed to serve a whole fragment.
			reader.skip(numbersSize);
		}

		return index;
	}

	std::string readFragment(std::istream& file, const std::uint64_t offset)
	{
		const std::uint64_t end = fileSize(file);
		if (offset >= end)
			throw FormatError("The fragment starts past the end of the file");

		file.seekg(static_cast<std::streamoff>(offset));
		std::string fragment;
		appendBox(file, end, "moof", fragment);
		appendBox(file, end, "mdat", fragment);
		return fragment;
	}

	FragmentParts splitFragment(const std::string_view fragment)
	{
		ByteReader reader(fragment);
		const std::uint64_t moofSize = reader.expectBox("moof");
		reader.skip(moofSize - kHeaderSize);
		const std::uint64_t mdatSize = reader.expectBox("mdat");
		return {fragment.substr(0, moofSize), reader.take(mdatSize - kHeaderSize)};
	}

	std::string joinFragment(const std::string_view moof, const std::string_view payload)
	{
		const auto mdatSize = static_cast<std::uint32_t>(kHeaderSize + payload.size());

		std::string fragment(moof);
		for (int shift = 24; shift >= 0; shift -= 8)
			fragment += static_cast<char>(mdatSize >> shift & 0xFF);
		fragment += "mdat";
		fragment += payload;
		return fragment;
	}
}
