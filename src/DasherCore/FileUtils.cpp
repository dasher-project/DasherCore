#ifndef HAVE_OWN_FILEUTILS
#include "FileUtils.h"

#include <regex>
#include <filesystem>
#include <fstream>

namespace Dasher {

// Static member initialization
std::string FileUtils::s_dataDirectory;

void FileUtils::SetDataDirectory(const std::string& dataDir) {
    s_dataDirectory = dataDir;
}

} // namespace Dasher

static bool IsFileWriteable(const std::filesystem::path& file_path) {
    std::fstream file(file_path.string(), std::ios_base::app | std::fstream::out);
    return file.is_open();
}

int Dasher::FileUtils::GetFileSize(const std::string& strFileName) {
    return static_cast<int>(std::filesystem::file_size(strFileName));
}

void Dasher::FileUtils::ScanFiles(AbstractParser* parser, const std::string& strPattern) {
    // Absolute path to a real file -> parse only that file
    std::error_code error_code; // just used for not throwing errors
    std::filesystem::path p(strPattern);
    if (p.is_absolute() && std::filesystem::exists(p, error_code) && std::filesystem::is_regular_file(p, error_code)) {
        parser->ParseFile(strPattern, IsFileWriteable(strPattern));
        return;
    }

    // Replace * with .* for actual regex matching
    // Note: pattern is interpreted as regex, so "alphabet.*.xml" matches "alphabet.English.xml"
    const std::regex pattern = std::regex(strPattern);

    // Search in the specified data directory (or current directory if not set)
    // Uses recursive_directory_iterator to find files in subdirectories
    std::vector<std::filesystem::path> search_paths;
    if (!s_dataDirectory.empty()) {
        search_paths.push_back(std::filesystem::path(s_dataDirectory));
    } else {
        search_paths.push_back(std::filesystem::current_path());
    }

    for (const std::filesystem::path& current_path : search_paths) {
        if (!std::filesystem::exists(current_path)) continue;
        // Use recursive_directory_iterator to search subdirectories
        for (const auto& entry : std::filesystem::recursive_directory_iterator(current_path)) {
            if (entry.is_regular_file() && std::regex_search(entry.path().filename().string(), pattern)) {
                parser->ParseFile(entry.path().string(), IsFileWriteable(entry.path()));
            }
        }
    }
}

bool Dasher::FileUtils::WriteUserDataFile(const std::string& filename, const std::string& strNewText, bool append) {
    std::filesystem::path fullPath(filename);
    if (fullPath.is_relative() && !s_dataDirectory.empty()) {
        fullPath = std::filesystem::path(s_dataDirectory) / filename;
    }
    std::ofstream File(fullPath, (append) ? std::ios_base::app : std::ios_base::out);

    if (File.is_open()) {
        File << strNewText;
        File.close();
        return true;
    }
    return false;
}

std::string Dasher::FileUtils::GetFullFilenamePath(const std::string strFilename) {
    // We get a weak canonical path in case the path does not exist
    std::filesystem::path path = std::filesystem::weakly_canonical(strFilename);

    return path.u8string(); // u8string to handle unicode characters.
}
#endif
