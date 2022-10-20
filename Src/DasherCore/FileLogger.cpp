#include "FileUtils.h"
#include "../Common/Common.h"

#include <string>
#include <filesystem>
#include <chrono>
#include "FileLogger.h"

CFileLogger::CFileLogger(const std::string& strFilenamePath, eLogLevel iLogLevel, int iOptionsMask) : m_iLogLevel(iLogLevel)
{
	// See what options are set in our bit mask options parameter
	if (iOptionsMask & logFunctionEntryExit)
		m_bFunctionLogging = true;
	if (iOptionsMask & logTimeStamp)
		m_bTimeStamp = true;
	if (iOptionsMask & logDateStamp)
		m_bDateStamp = true;
	if (iOptionsMask & logDeleteOldFile)
		m_bDeleteOldFile = true;
	if (iOptionsMask & logFunctionTiming)
		m_bFunctionTiming = true;
	if (iOptionsMask & logOutputScreen)
		m_bOutputScreen = true;

	// On windows anyway if somebody can open up a file with CreateFile() in a different 
	// directory and cause the working directory to change.  We don't want our log file
	// moving around, so we'll find a absolute path when we are created and stick to
	// that for the remainder of our life.
	m_strFilenamePath = Dasher::FileUtils::GetFullFilenamePath(strFilenamePath);

	// See if we should get rid of any existing filename with our given name.  This prevents having
	// to remember to delete the file before every new debugging run.
	if (m_bDeleteOldFile)  
	{
		Dasher::FileUtils::WriteUserDataFile(strFilenamePath, "", false); //Delete file contents. Consistent with old behavior.
	}
}

CFileLogger::~CFileLogger()
{

	if (!m_bFunctionTiming) return;

	// Dump the results of our function timing logging
	Log("%-60s%20s%10s", logNORMAL, "Function","Ticks", "Percent");
	Log("%-60s%20s%10s", logNORMAL, "--------","-----", "-------");

	// First pass to count the max ticks 
	// We assume that there was a function logger on the outer most (main) program.
	// This allows the percent reflect the relative time spent inside embedded calls.

	double max_duration = 0;
	for(const auto &[function_name, duration] : m_mapFunctionDuration)
	{
	    if(duration > max_duration) max_duration = duration;
	}

	for(const auto &[function_name, duration] : m_mapFunctionDuration)
	{
		Log("%-60s%20I64Ld%10.2f", logNORMAL, function_name.c_str(), duration, static_cast<double>(duration) / max_duration  * 100.0);
	}
}

// Changes the filename of this logging object
void CFileLogger::SetFilename(const std::string& strFilename)
{
	m_strFilenamePath = strFilename;

	// See if we should get rid of any existing filename with our given name.  This prevents having
	// to remember to delete the file before every new debugging run.
	if (m_bDeleteOldFile)
	{
		Dasher::FileUtils::WriteUserDataFile(strFilename, "", false); //Delete file contents. Consistent with old behavior.
	}
}

void CFileLogger::Log(const char* szText, va_list args)
{
	if (m_strFilenamePath.length() <= 0 && szText == nullptr) return;

	std::string strIndented = GetTimeDateStamp() + " " + GetIndentedString(szText) + "\n";

    std::string logLine;

	int length = vsnprintf(nullptr, 0, strIndented.c_str(), args);
	logLine.resize(length);
	vsnprintf(&logLine[0], length + 1, strIndented.c_str(), args);
	
    Dasher::FileUtils::WriteUserDataFile(m_strFilenamePath, logLine, true);

	// Optionally we can output message to stdout
	if (m_bOutputScreen)
	{
		std::cout << logLine;
    }
}

// Logs a string to our file.  eLogLevel specifies the importance of this message, we
// only write to the log file if it is at least as important as the level set in the 
// constructor.  Accepts printf style formatting in the first string which must be 
// filled with the variable parameter list at the end.
// NOTE: Currently not thread safe!
void CFileLogger::Log(const char* szText, eLogLevel iLogLevel, ...)
{
	if(m_iLogLevel > iLogLevel) return;

    va_list args;  

	va_start(args, iLogLevel);
		Log(szText, args);
	va_end(args);
}

// Overloaded version that takes a STD::string
void CFileLogger::Log(const std::string strText, eLogLevel iLogLevel, ...)
{
	if(m_iLogLevel > iLogLevel) return;

	va_list args;  

	va_start(args, iLogLevel);
		Log(strText.c_str(), args);
	va_end(args);
}

// Version that assume log level is logDEBUG
void CFileLogger::LogDebug(const char* szText, ...)
{
	if(m_iLogLevel > logDEBUG) return;

    va_list args;  

	va_start(args, szText);
		Log(szText, args);
	va_end(args);
}

// Version that assume log level is logNormal
void CFileLogger::LogNormal(const char* szText, ...)
{
    if(m_iLogLevel > logNORMAL) return;

    va_list args;  

	va_start(args, szText);
		Log(szText, args);
	va_end(args);
}

// Version that assume log level is logCRITICAL
void CFileLogger::LogCritical(const char* szText, ...)
{
    va_list args;  

	va_start(args, szText);
		Log(szText, args);
	va_end(args);
}

// Logs entry into a particular function
void CFileLogger::LogFunctionEntry(const std::string& strFunctionName)
{
	if (m_bFunctionLogging)
	{
		Log("start: " + strFunctionName);
		m_iFunctionIndentLevel++;
	}
}

// Logs exit into a particular function
void CFileLogger::LogFunctionExit(const std::string& strFunctionName)
{
	if (m_bFunctionLogging)
	{
		m_iFunctionIndentLevel--;
		Log("end: " + strFunctionName);
	}
}


void CFileLogger::LogFunctionTicks(const std::string& strFunctionName, double duration)
{
	m_mapFunctionDuration[strFunctionName] += duration;
}

// Gets an indented version of the function name 
std::string CFileLogger::GetIndentedString(const std::string& strText)
{
	std::string indentation(m_iFunctionIndentLevel, ' ');
	return indentation + strText;
}

bool CFileLogger::GetFunctionTiming()
{
	return m_bFunctionTiming;
}


// Update what log level this object is using
void CFileLogger::SetLogLevel(const eLogLevel iNewLevel)
{
	m_iLogLevel = iNewLevel;
}

// Update whether function entry/exit is logged
void CFileLogger::SetFunctionLogging(bool bFunctionLogging)
{
	m_bFunctionLogging = bFunctionLogging;
}


// Gets the time and/or date stamp as specified
// by our construction options.

// Format is:
// Oct 19 2022 18:35:12.862
std::string CFileLogger::GetTimeDateStamp()
{
	std::string strTimeStamp;
	std::string format;
	if(m_bDateStamp)
	{
		format = "%b %d %Y";
	}
	if(m_bTimeStamp)
	{
		if(m_bDateStamp) format += " ";
		format += "%H:%M:%S";
	}
	
	if (m_bTimeStamp || m_bDateStamp)
	{
		std::chrono::time_point<chrono::system_clock> timepoint = std::chrono::system_clock::now();
	    std::time_t now = std::chrono::system_clock::to_time_t(timepoint);
	    int milliseconds = static_cast<int>(std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint).time_since_epoch().count() % 1000);
		std::string strMillis(3,'0');
		sprintf(&strMillis[0], "%03d", milliseconds);

		std::string Buffer(30, '\0'); //never longer than 30 chars
		size_t length = std::strftime(&Buffer[0], Buffer.size(), format.c_str(), std::localtime(&now)); //Not thread safe!
		Buffer.resize(length); //Resize to strip \0 characters depending on format
		if(m_bTimeStamp) Buffer += "." + strMillis;
		return Buffer;
	}

	return strTimeStamp;
}

/////////////////////////////////////// CFunctionLogger /////////////////////////////////////////////////////////////

CFunctionLogger::CFunctionLogger(const std::string& strFunctionName, CFileLogger* pLogger) : m_pLogger(pLogger)
{
	if (m_pLogger == nullptr && strFunctionName.length() <= 0) return;

	m_strFunctionName = strFunctionName;

	if (!m_pLogger->GetFunctionTiming())
	{
		m_pLogger->LogFunctionEntry(m_strFunctionName);
	}
	else
	{
	    m_startTime = chrono::steady_clock::now();
	}
}

CFunctionLogger::~CFunctionLogger()
{
	if (m_pLogger == nullptr && m_strFunctionName.length() <= 0) return;

    if (!m_pLogger->GetFunctionTiming())
    {
	    m_pLogger->LogFunctionExit(m_strFunctionName);
    }
    else
    {
	    const auto current_time = chrono::steady_clock::now();
	    const auto span = std::chrono::duration_cast<chrono::duration<double>>(current_time - m_startTime);
		// Add our total ticks to the tracking map object in the logger object
		m_pLogger->LogFunctionTicks(m_strFunctionName, span.count());
    }
}