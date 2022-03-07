#include "FileUtils.h"
#include <filesystem>
#include <regex>

int Dasher::FileUtils::GetFileSize(const std::string& strFileName)
{
	return static_cast<int>(std::filesystem::file_size(strFileName));
}

void Dasher::FileUtils::ScanFiles(AbstractParser* parser, const std::string& strPattern)
{
	const std::regex Pattern = std::regex(strPattern);
		
	for (const auto & entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
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
