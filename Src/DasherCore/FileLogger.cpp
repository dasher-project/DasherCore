
#include "../Common/Common.h"

#include <cstring>
#include <filesystem>
#include <chrono>
#include "FileLogger.h"




CFileLogger::CFileLogger(const std::string& strFilenamePath, eLogLevel iLogLevel, int iOptionsMask)
{
  m_strFilenamePath       = "";
  m_iLogLevel             = iLogLevel;
  m_iFunctionIndentLevel  = 0;

  m_bFunctionLogging      = false;
  m_bTimeStamp            = false;
  m_bDateStamp            = false;
  m_bDeleteOldFile        = false;
  m_bFunctionTiming       = false;
  m_bOutputScreen         = false;

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
  m_strFilenamePath = GetFullFilenamePath(strFilenamePath);

  // See if we should get rid of any existing filename with our given name.  This prevents having
  // to remember to delete the file before every new debugging run.
  if (m_bDeleteOldFile)
  {
    FILE* fp = NULL;

    fp = fopen(m_strFilenamePath.c_str(), "w");
    if (fp != NULL)
    {
      fclose(fp);
      fp = NULL;
    }
  }

}

CFileLogger::~CFileLogger()
{
#ifdef _WIN32
  if (m_bFunctionTiming)
  {
    // Dump the results of our function timing logging

    MAP_STRING_INT64::iterator map_iter;

    Log("%-60s%20s%10s", logNORMAL, "Function","Ticks", "Percent");
    Log("%-60s%20s%10s", logNORMAL, "--------","-----", "-------");

    __int64 iMaxTicks = 0;

    // First pass to count the max ticks 
    // We assume that there was a function logger on the outer most (main) program.
    // This allows the percent reflect the relative time spent inside emedded calls.

    for (map_iter = m_mapFunctionTicks.begin(); map_iter != m_mapFunctionTicks.end(); map_iter++)
    {
      if (map_iter->second > iMaxTicks)
        iMaxTicks = map_iter->second;
    }

    for (map_iter = m_mapFunctionTicks.begin(); map_iter != m_mapFunctionTicks.end(); map_iter++)
    {
      std::string name = map_iter->first;
      __int64 iTicks = map_iter->second;
      Log("%-60s%20I64Ld%10.2f", logNORMAL, name.c_str(), iTicks, (double) iTicks / (double) iMaxTicks  * (double) 100.0);
    }
  }
#endif
}

// Changes the filename of this logging object
void CFileLogger::SetFilename(const std::string& strFilename)
{
  m_strFilenamePath = strFilename;

  // See if we should get rid of any existing filename with our given name.  This prevents having
  // to remember to delete the file before every new debugging run.
  if (m_bDeleteOldFile)
  {
	  //old implementation didn't care if a file was actually deleted, so we keep the behaviour
      std::filesystem::remove(strFilename);
  }
}

// Logs a string to our file.  eLogLevel specifies the importance of this message, we
// only write to the log file if it is at least as important as the level set in the 
// constructor.  Accepts printf style formatting in the first string which must be 
// filled with the variable parameter list at the end.
// NOTE: Currently not thread safe!
void CFileLogger::Log(const char* szText, eLogLevel iLogLevel, ...)
{
  va_list       args;  

  if ((m_strFilenamePath.length() > 0) && 
      (m_iLogLevel <= iLogLevel) && 
      (szText != NULL))
  {
    FILE* fp = NULL;
    fp = fopen(m_strFilenamePath.c_str(), "a");

    std::string strTimeStamp = GetTimeDateStamp();

    if (fp != NULL)
    {
      std::string strIndented = strTimeStamp + GetIndentedString(szText) + "\n";

      va_start(args, iLogLevel);
      vfprintf(fp, strIndented.c_str(), args);
      va_end(args);

      // Optionally we can output message to stdout
      if (m_bOutputScreen)
      {
        va_start(args, iLogLevel);
        vprintf(strIndented.c_str(), args);
        va_end(args);
      }

      fclose(fp);
      fp = NULL;
    }
  }
}

// Overloaded version that takes a STL::string
void CFileLogger::Log(const std::string strText, eLogLevel iLogLevel, ...)
{
  va_list       args;  

  if ((m_strFilenamePath.length() > 0) && 
      (m_iLogLevel <= iLogLevel))
  {
    FILE* fp = NULL;
    fp = fopen(m_strFilenamePath.c_str(), "a");

    std::string strTimeStamp = GetTimeDateStamp();

    if (fp != NULL)
    {
      std::string strIndented = strTimeStamp + GetIndentedString(strText) + "\n";

      va_start(args, iLogLevel);
      vfprintf(fp, strIndented.c_str(), args);
      va_end(args);

      // Optionally we can output message to stdout
      if (m_bOutputScreen)
      {
        va_start(args, iLogLevel);
        vprintf(strIndented.c_str(), args);
        va_end(args);
      }

      fclose(fp);
      fp = NULL;
    }
  }
}

// Version that assume log level is logDEBUG
void CFileLogger::LogDebug(const char* szText, ...)
{
  // Note: it would be nice not to duplicate code, but the variable
  // parameter list makes this problematic.
  va_list       args;  

  if ((m_strFilenamePath.length() > 0) && 
      (m_iLogLevel == logDEBUG) &&
      (szText != NULL))
  {
    FILE* fp = NULL;
    fp = fopen(m_strFilenamePath.c_str(), "a");

    std::string strTimeStamp = GetTimeDateStamp();

    if (fp != NULL)
    {
      std::string strIndented = strTimeStamp + GetIndentedString(szText) + "\n";

      va_start(args, szText);
      vfprintf(fp, strIndented.c_str(), args);
      va_end(args);

      // Optionally we can output message to stdout
      if (m_bOutputScreen)
      {
        va_start(args, szText);
        vprintf(strIndented.c_str(), args);
        va_end(args);
      }

      fclose(fp);
      fp = NULL;
    }
  }
}

// Version that assume log level is logNormal
void CFileLogger::LogNormal(const char* szText, ...)
{
  // Note: it would be nice not to duplicate code, but the variable
  // parameter list makes this problematic.
  va_list       args;  

  if ((m_strFilenamePath.length() > 0) && 
      (m_iLogLevel <= logNORMAL) &&
      (szText != NULL))
  {
    FILE* fp = NULL;
    fp = fopen(m_strFilenamePath.c_str(), "a");

    std::string strTimeStamp = GetTimeDateStamp();

    if (fp != NULL)
    {
      std::string strIndented = strTimeStamp + GetIndentedString(szText) + "\n";

      va_start(args, szText);
      vfprintf(fp, strIndented.c_str(), args);
      va_end(args);

      // Optionally we can output message to stdout
      if (m_bOutputScreen)
      {
        va_start(args, szText);
        vprintf(strIndented.c_str(), args);
        va_end(args);
      }

      fclose(fp);
      fp = NULL;
    }
  }
}

// Version that assume log level is logCRITICAL
void CFileLogger::LogCritical(const char* szText, ...)
{
  // Note: it would be nice not to duplicate code, but the variable
  // parameter list makes this problematic.
  va_list       args;  

  // Always log critical messages
  if ((m_strFilenamePath.length() > 0) &&
      (szText != NULL))
  {
    FILE* fp = NULL;
    fp = fopen(m_strFilenamePath.c_str(), "a");

    std::string strTimeStamp = GetTimeDateStamp();

    if (fp != NULL)
    {
      std::string strIndented = strTimeStamp + GetIndentedString(szText) + "\n";

      va_start(args, szText);
      vfprintf(fp, strIndented.c_str(), args);
      va_end(args);

      // Optionally we can output message to stdout
      if (m_bOutputScreen)
      {
        va_start(args, szText);
        vprintf(strIndented.c_str(), args);
        va_end(args);
      }

      fclose(fp);
      fp = NULL;
    }
  }
}

// Logs entry into a particular function
void CFileLogger::LogFunctionEntry(const std::string& strFunctionName)
{
  if (m_bFunctionLogging)
  {
    std::string strStart = "start: ";
    strStart += strFunctionName;
    Log(strStart.c_str());
    m_iFunctionIndentLevel++;
  }
}

// Logs exit into a particular function
void CFileLogger::LogFunctionExit(const std::string& strFunctionName)
{
  if (m_bFunctionLogging)
  {
    m_iFunctionIndentLevel--;
    std::string strEnd = "end: ";
    strEnd += strFunctionName;
    Log(strEnd.c_str());
  }
}

#ifdef _WIN32
void CFileLogger::LogFunctionTicks(const std::string& strFunctionName, __int64 iTicks)
{
  __int64 iCurrent;

  iCurrent = m_mapFunctionTicks[strFunctionName];
  iCurrent = iCurrent + iTicks;

  m_mapFunctionTicks[strFunctionName] = iCurrent;
}
#endif

// Gets an indented version of the function name 
std::string CFileLogger::GetIndentedString(const std::string& strText)
{
  std::string strIndented = "";
  for (int i = 0; i < m_iFunctionIndentLevel; i++)
    strIndented += " ";
  strIndented += strText;

  return strIndented;
}

bool CFileLogger::GetFunctionTiming()
{
  return m_bFunctionTiming;
}

// Utility method that converts a filename into a fully qualified
// path and filename on Windows.  This can be used to make sure
// a relative filename stays pointed at the same location despite
// changes in the working directory.
// UPDATED: Replaced windows only method with portable code using the standard library.
std::string CFileLogger::GetFullFilenamePath(std::string strFilename)
{
  auto path = std::filesystem::path(strFilename);
  std::string canonicalPath  = std::filesystem::canonical(path).u8string();  //u8string to handle unicode characters.
	
  return canonicalPath;
}

/////////////////////////////////////// CFunctionLogger /////////////////////////////////////////////////////////////

CFunctionLogger::CFunctionLogger(const std::string& strFunctionName, CFileLogger* pLogger) 
{
  m_pLogger = pLogger;

  if ((m_pLogger != NULL) && (strFunctionName.length() > 0))
  {
    m_strFunctionName = strFunctionName;

    if (!m_pLogger->GetFunctionTiming())
      m_pLogger->LogFunctionEntry(m_strFunctionName);
    else {
#ifdef _WIN32
      QueryPerformanceCounter(&m_iStartTicks);
#endif
    }
  }

}

CFunctionLogger::~CFunctionLogger()
{
  if ((m_pLogger != NULL) && (m_strFunctionName.length() > 0))
  {
    if (!m_pLogger->GetFunctionTiming())
      m_pLogger->LogFunctionExit(m_strFunctionName);
    else
    {
#ifdef _WIN32
      LARGE_INTEGER iEndTicks;
      QueryPerformanceCounter(&iEndTicks);
      // Add our total ticks to the tracking map object in the logger object
	  m_pLogger->LogFunctionTicks(m_strFunctionName,
		  iEndTicks.QuadPart - m_iStartTicks.QuadPart);
#endif
    }
  }
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
std::string CFileLogger::GetTimeDateStamp()
{
  std::string strTimeStamp;
  auto format = "";
  if(m_bTimeStamp)
  {
      format = "%T";
  }
  if(m_bDateStamp)
  {
      format = "%b %m %Y"; //format as specified by dasher " abbreviated month name, month, year"
  }
	

  if ((m_bTimeStamp) || (m_bDateStamp))
  {  	
      char buf[80];
      const auto timestamp = std::chrono::system_clock::now();
      auto timestamp_t = std::chrono::system_clock::to_time_t(timestamp);
      strftime(buf, sizeof(buf), format, std::localtime(&timestamp_t));
  }

  return strTimeStamp;
}
