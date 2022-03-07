
#pragma once
#include <DasherInterfaceBase.h>
#include <Messages.h>
#include <iostream>

namespace Dasher {

//needed File utilities
class FileUtils : public CFileUtils {
public:
	//Return file size on disk
	virtual int GetFileSize(const std::string& strFileName) override;

	//Open File with the filename strPattern in the project directory
	virtual void ScanFiles(AbstractParser* parser, const std::string& strPattern) override;

	//Writes into the user file 
	virtual bool WriteUserDataFile(const std::string& filename, const std::string& strNewText, bool append) override;
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