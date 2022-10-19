#ifndef HAVE_OWN_FILEUTILS
#include "FileUtils.h"
#include <regex>
#include <filesystem>

/* Taken from https://stackoverflow.com/a/69950065 */
static bool IsFileWriteable(const std::filesystem::path &file_path)
{
    FILE* file_handle;
    if (fopen_s(&file_handle, file_path.string().c_str(), "a") != 0)
    {
        return false;
    }

    fclose(file_handle);
    return true;
}

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
	return static_cast<int>(std::filesystem::file_size(strFileName));
}

void Dasher::FileUtils::ScanFiles(AbstractParser* parser, const std::string& strPattern)
{
	// Full real path given -> parse only that file
	std::error_code error_code; // just used for not throwing errors
	if(std::filesystem::exists(strPattern, error_code) && std::filesystem::is_regular_file(strPattern, error_code))
	{
		parser->ParseFile(strPattern, IsFileWriteable(strPattern));
		return;
	}

	// Replace * with .* for actual regex matching
	std::string alteredPattern = StringReplaceAll(strPattern, "*", ".*");
	const std::regex pattern = std::regex(alteredPattern);

	// Search in predefined directories for the files. Currently it is searched in {".", "./Data"} (relative to the working directory)
	for(const std::filesystem::path& current_path : {std::filesystem::current_path(), std::filesystem::current_path() / "Data"})
	{
		for (const auto & entry : std::filesystem::directory_iterator(current_path))
		{
			if (entry.is_regular_file() && std::regex_search(entry.path().filename().string(), pattern))
			{
				parser->ParseFile(entry.path().string(), IsFileWriteable(entry.path()));
			}
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

std::string Dasher::FileUtils::GetFullFilenamePath(const std::string strFilename)
{
	//We get a weak canonical path in case the path does not exist
	std::filesystem::path path = std::filesystem::weakly_canonical(strFilename);

	return path.u8string(); //u8string to handle unicode characters.
}
#endif
