#ifndef HAVE_OWN_FILEUTILS
#include "FileUtils.h"

#include <regex>
#include <filesystem>
#include <fstream>

namespace Dasher {

// Static member initialization
std::string FileUtils::s_dataDirectory;
std::string FileUtils::s_userDataDirectory;

void FileUtils::SetDataDirectory(const std::string& dataDir) {
    s_dataDirectory = dataDir;
}

void FileUtils::SetUserDataDirectory(const std::string& userDir) {
    s_userDataDirectory = userDir;
}

} // namespace Dasher

static bool IsFileWriteable(const std::filesystem::path& file_path) {
    // Check writability via permission bits without opening the file.
    // Opening with ios_base::app|out (the previous implementation) attempted
    // to create/append the file just to test writability, which generated
    // sandbox deny storms against read-only bundled data on iOS keyboard
    // extensions and could accidentally create empty files on other platforms.
    std::error_code ec;
    const auto status = std::filesystem::status(file_path, ec);
    if (ec) return false;
    using P = std::filesystem::perms;
    return (status.permissions() & (P::owner_write | P::group_write | P::others_write)) != P::none;
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
    std::ofstream File(ResolveUserDataPath(filename), (append) ? std::ios_base::app : std::ios_base::out);

    if (File.is_open()) {
        File << strNewText;
        File.close();
        return true;
    }
    return false;
}

std::string Dasher::FileUtils::ResolveUserDataPath(const std::string& filename) {
    std::filesystem::path fullPath(filename);
    if (!fullPath.is_relative()) return filename;
    // Prefer the user-writable data directory for mutable files
    // (training deltas, dasher.log, settings). Fall back to the bundled
    // data directory if the client never configured a separate user dir
    // — this preserves the historical single-dir behaviour.
    if (!s_userDataDirectory.empty()) {
        return (std::filesystem::path(s_userDataDirectory) / filename).string();
    }
    if (!s_dataDirectory.empty()) {
        return (std::filesystem::path(s_dataDirectory) / filename).string();
    }
    return filename;
}

std::string Dasher::FileUtils::GetFullFilenamePath(const std::string strFilename) {
    // We get a weak canonical path in case the path does not exist
    std::filesystem::path path = std::filesystem::weakly_canonical(strFilename);

    return path.u8string(); // u8string to handle unicode characters.
}
#endif
