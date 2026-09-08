#pragma once
#include "DasherCore/AbstractXMLParser.h"
#include "DasherCore/Messages.h"
#include <iostream>

namespace Dasher {

// needed File utilities
class FileUtils {
  public:
    // Set the data directory for file operations (read-only bundled data:
    // alphabets, training corpora, etc.). Used by ScanFiles().
    static void SetDataDirectory(const std::string& dataDir);

    // Set the user data directory for writable per-user files (training
    // text deltas, dasher.log, settings). Must point to a writable
    // location. Falls back to the data directory if never set, preserving
    // historical behaviour for clients that don't distinguish the two.
    static void SetUserDataDirectory(const std::string& userDir);

    // Return file size on disk
    static int GetFileSize(const std::string& strFileName);

    // Open File with the filename strPattern in the project directory
    static void ScanFiles(AbstractParser* parser, const std::string& strPattern);

    // Scan a specific directory (recursively) instead of the process-global
    // data directory. Same pattern semantics as ScanFiles; an empty dir
    // scans nothing. Lets per-context callers (NodeCreationManager training
    // load, dasher-project/DasherCore#84) avoid the last-created-context
    // globals that ScanFiles consults.
    static void ScanDirectory(AbstractParser* parser, const std::string& strPattern, const std::string& dir);

    // True when two directory paths name the same location: uses
    // std::filesystem::equivalent when both exist (symlinks, junctions,
    // case-insensitive roots), falling back to weakly_canonical lexical
    // comparison for not-yet-created dirs. Empty paths never match.
    static bool IsSameDirectory(const std::string& a, const std::string& b);

    // Writes into the user file
    static bool WriteUserDataFile(const std::string& filename, const std::string& strNewText, bool append);

    // Resolve a relative filename against the user data directory (or, if
    // unset, the data directory). Returns the input unchanged if absolute.
    // Used by code that needs the resolved path up front (e.g. FileLogger
    // captures its path at construction time).
    static std::string ResolveUserDataPath(const std::string& filename);

    // Convert relative to full paths
    static std::string GetFullFilenamePath(const std::string strFilename);

  private:
    static std::string s_dataDirectory;
    static std::string s_userDataDirectory;
};

// Just a function to Log XML errors
class CommandlineErrorDisplay : public CMessageDisplay {
  public:
    virtual ~CommandlineErrorDisplay() = default;

    void Message(const std::string& strText, bool) override { std::cout << strText << std::endl; }
};

} // namespace Dasher