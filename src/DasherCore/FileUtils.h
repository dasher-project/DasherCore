#pragma once
#include "DasherCore/AbstractXMLParser.h"
#include "DasherCore/Messages.h"
#include <iostream>

namespace Dasher {

//needed File utilities
class FileUtils {
public:
	//Set the data directory for file operations
	static void SetDataDirectory(const std::string& dataDir);

	//Return file size on disk
	static int GetFileSize(const std::string& strFileName);

	//Open File with the filename strPattern in the project directory
	static void ScanFiles(AbstractParser* parser, const std::string& strPattern);

	//Writes into the user file
	static bool WriteUserDataFile(const std::string& filename, const std::string& strNewText, bool append);

	//Convert relative to full paths
	static std::string GetFullFilenamePath(const std::string strFilename);

private:
	static std::string s_dataDirectory;
};



//Just a function to Log XML errors
class CommandlineErrorDisplay : public CMessageDisplay {
public:
	virtual ~CommandlineErrorDisplay() = default;

	void Message(const std::string& strText, bool) override {
		std::cout << strText << std::endl;
	}
};

}