#pragma once

#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <Poco/File.h>
#include <Poco/Path.h>

namespace qs
{
	/// Reads a whole file as bytes.
	/// @param path The file to read.
	/// @return The contents, or nothing when the file cannot be opened.
	inline std::optional<std::string> readFile(const std::string& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
			return std::nullopt;

		return std::string(std::istreambuf_iterator<char>(stream), {});
	}

	/// Resolves a path relative to a folder.
	/// @param folder The folder the path is relative to.
	/// @param relative The relative path.
	inline Poco::Path resolvePath(const Poco::Path& folder, const std::string& relative)
	{
		Poco::Path path(folder);
		path.append(relative);
		return path;
	}

	/// Finds the folder of an episode under the episodes path.
	/// @param episodesPath The folder holding one subfolder per episode.
	/// @param episodeId The episode to look for.
	/// @return The episode's folder, or nothing when it does not exist.
	inline std::optional<Poco::Path> findEpisodeFolder(const std::string& episodesPath, const std::string& episodeId)
	{
		Poco::Path folder = resolvePath(Poco::Path(episodesPath), episodeId);
		if (const Poco::File file(folder); !file.exists() || !file.isDirectory())
			return std::nullopt;

		return folder;
	}
}
