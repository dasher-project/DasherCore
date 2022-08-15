#ifndef HAVE_OWN_FILEUTILS
#include "FileUtils.h"
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

int Dasher::FileUtils::GetFileSize(const std::string& strFileName)
{
	return static_cast<int>(fs::file_size(strFileName));
}

void Dasher::FileUtils::ScanFiles(AbstractParser* parser, const std::string& strPattern)
{
	const std::regex Pattern = std::regex(strPattern);
		
	for (const auto & entry : fs::directory_iterator(fs::current_path()))
	{
		if (entry.is_character_file() && std::regex_match(entry.path().filename().string(), Pattern))
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