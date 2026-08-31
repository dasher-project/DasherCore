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
    // Absolute paths name one specific file, never a glob. If the file is
    // missing, return instead of falling through to the regex: compiling a
    // Windows path as a regex throws on its backslashes ("\2" is parsed as
    // a backreference, "\c" as an invalid control escape), which made
    // dasher_create fail whenever the settings file did not exist yet
    // (fresh user dir) — XmlSettingsStore::Load() ScanFiles()s the
    // settings path itself.
    std::error_code error_code; // just used for not throwing errors
    std::filesystem::path p(strPattern);
    if (p.is_absolute()) {
        if (std::filesystem::exists(p, error_code) && std::filesystem::is_regular_file(p, error_code)) {
            parser->ParseFile(strPattern, IsFileWriteable(strPattern));
        }
        return;
    }

    // Replace * with .* for actual regex matching
    // Note: pattern is interpreted as regex, so "alphabet.*.xml" matches "alphabet.English.xml"
    const std::regex pattern = std::regex(strPattern);

    // Search ONLY in the specified data directory. The old fallback to
    // current_path() turned an empty/missing data dir (frontend passed a bad
    // bundle path) into an unbounded scan of the user's home directory —
    // minutes of regex per file while the caller held its engine lock
    // (watch spike hang, 2026-08-31). No data dir means nothing to scan.
    std::vector<std::filesystem::path> search_paths;
    if (!s_dataDirectory.empty()) {
        search_paths.push_back(std::filesystem::path(s_dataDirectory));
    }

    for (const std::filesystem::path& current_path : search_paths) {
        std::error_code exists_ec;
        if (!std::filesystem::exists(current_path, exists_ec) || exists_ec) continue;
        // Iterative walk that isolates failures PER DIRECTORY: one
        // inaccessible or transiently failing subtree skips just that
        // subtree; sibling directories still scan (review P1 on #77 — a
        // mid-iteration error previously ended the whole search root, and the
        // throwing overloads before that aborted Realize entirely). Symlinks
        // are never followed — matches recursive_directory_iterator's default
        // and avoids cycles.
        std::vector<std::filesystem::path> pending{current_path};
        while (!pending.empty()) {
            const std::filesystem::path dir = pending.back();
            pending.pop_back();
            std::error_code it_ec;
            for (std::filesystem::directory_iterator it(dir, it_ec), end; !it_ec && it != end;
                 it.increment(it_ec)) {
                std::error_code ent_ec;
                const std::filesystem::path p = it->path();
                if (it->is_symlink(ent_ec) || ent_ec) continue;
                if (it->is_directory(ent_ec) && !ent_ec) {
                    pending.push_back(p);
                    continue;
                }
                ent_ec.clear();
                if (it->is_regular_file(ent_ec) && !ent_ec &&
                    std::regex_search(p.filename().string(), pattern)) {
                    parser->ParseFile(p.string(), IsFileWriteable(p));
                }
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
