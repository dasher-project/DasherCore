
// Some very simple XML utility functions that just function on STL strings.
//
// Good for recursively splitting up the job of reinstatiating objects
// that have been serialized to an XML file.
//
// Copyright 2005 by Keith Vertanen

#ifndef __XML_UTIL_H__
#define __XML_UTIL_H__

#include <cstdlib>
#include <string>
#include <vector>
#include <stdio.h>
#include "FileLogger.h"

extern CFileLogger* gLogger;


#ifndef VECTOR_STRING
typedef std::vector<std::string> VECTOR_STRING;
#endif
#ifndef VECTOR_STRING_ITER
typedef std::vector<std::string>::iterator VECTOR_STRING_ITER;
#endif

// We want to be able grab all the name/value pairs in XML like:
//  <Params>
//      <Color>red</Color>
//      <Size>10</Size>
//  </Params>
struct NameValuePair
{
  std::string  strName;
  std::string  strValue;
};

typedef std::vector<NameValuePair>               VECTOR_NAME_VALUE_PAIR;
typedef std::vector<NameValuePair>::iterator     VECTOR_NAME_VALUE_PAIR_ITER;

const int XML_UTIL_READ_BUFFER_SIZE     = 4096;
const int XML_UTIL_DEFAULT_VECTOR_SIZE  = 128;
/// \ingroup Logging
/// @{
class XMLUtil
{
public:
  XMLUtil();

  static std::string			              LoadFile(const std::string& filename, unsigned int iSizeHint = 128000);
  static std::string			              GetElementString(const std::string& strTag, const std::string& strXML, bool bStripWhiteSpace = true);
  static int				              GetElementInt(const std::string& strTag, const std::string& strXML, bool* pFound = NULL);
  static long long		            GetElementLongLong(const std::string& strTag, const std::string& strXML, bool* pFound = NULL);
  static float			              GetElementFloat(const std::string& strTag, const std::string& strXML, bool* pFound = NULL);
  static VECTOR_STRING	          GetElementStrings(const std::string& strTag, const std::string& strXML, bool bStripWhiteSpace = true);
  static VECTOR_NAME_VALUE_PAIR   GetNameValuePairs(const std::string& strXML, bool bStripWhiteSpace = true);

  static bool				        IsWhiteSpace(char cLetter);
  static std::string			        StripWhiteSpace(const std::string& strText);
  static bool				        IsDigit(char cLetter);

};
/// @}

#endif

