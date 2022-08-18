#ifndef HAVE_OWN_FILEUTILS
#include "FileUtils.h"
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

/* Taken from https://stackoverflow.com/a/24315631 */
static std::string StringReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

int Dasher::FileUtils::GetFileSize(const std::string& strFileName)
{
	return static_cast<int>(fs::file_size(strFileName));
}

void Dasher::FileUtils::ScanFiles(AbstractParser* parser, const std::string& strPattern)
{
	// Replace * with .* for actual regex matching
	std::string alteredPattern = StringReplaceAll(strPattern, "*", ".*");

	const std::regex Pattern = std::regex(alteredPattern);
	for (const auto & entry : fs::directory_iterator(fs::current_path()))
	{
		if (entry.is_regular_file() && std::regex_search(entry.path().filename().string(), Pattern))
		{
			parser->ParseFile(entry.path().string(), true);
		}
	}
}

bool Dasher::FileUtils::WriteUserDataFile(const std::string& filename, const std::string& strNewText, bool append)
{
	ofstream File(filename, (append) ? std::ios_base::app : ios_base::out);

	if(File.is_open())
	{
		File << strNewText;
		File.close();
		return true;
	}
	return false;
}
#endif