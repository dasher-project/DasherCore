#include <cstring>

#include "XMLUtil.h"
#include <fstream>


bool XMLUtil::IsWhiteSpace(char cLetter)
{
  if ((cLetter == ' ') ||
      (cLetter == '\n') ||
      (cLetter == '\r') ||
      (cLetter == '\t'))
    return true;

  return false;
}

// See if a character is 0 - 9
bool XMLUtil::IsDigit(char cLetter)
{
  if ((cLetter >= '0') && (cLetter <= '9'))
    return true;
  return false;
}

// Strip the leading and trailing white space off a string.
std::string XMLUtil::StripWhiteSpace(const std::string& strText)
{
  std::string strResult = "";

  strResult.reserve(strText.length());

  int iStart = 0;
  while ((iStart < (int) strText.length()) && (IsWhiteSpace(strText[iStart])))
    iStart++;

  int iEnd = static_cast<int>(strText.length()) - 1;
  while ((iEnd > 0) && (IsWhiteSpace(strText[iEnd])))
    iEnd--;

  strResult = strText.substr(iStart, iEnd - iStart + 1);

  return strResult;
}

// Return a string containing the contents of a file
std::string XMLUtil::LoadFile(const std::string& strFilename, unsigned int iSizeHint)
{
  std::ifstream ifs(strFilename);
  std::string strResult((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	

  return strResult;
}


// Returns what is between the given tag in the passed XML.  We only return the first matching
// tag if there are multiple in the XML.  Tags are case sensitive.
std::string XMLUtil::GetElementString(const std::string& strTag, const std::string& strXML, bool bStripWhiteSpace)
{
  std::string strResult = "";
  std::string strStart = "";
  std::string strEnd = "";

  strStart += "<";
  strStart += strTag;
  strStart += ">";

  strEnd += "</";
  strEnd += strTag;
  strEnd += ">";

  int iPosStart = static_cast<int>(strXML.find(strStart));
  int iPosEnd = static_cast<int>(strXML.find(strEnd));

  if ((iPosStart != -1) && (iPosEnd != -1))
  {
    strResult = strXML.substr(iPosStart + strStart.length(), iPosEnd - (iPosStart + strStart.length()));
  }

  if (bStripWhiteSpace)
    strResult = StripWhiteSpace(strResult);

  return strResult;
}

// Return the integer representing an element
int XMLUtil::GetElementInt(const std::string& strTag, const std::string& strXML, bool* pFound)
{
  std::string strElement = GetElementString(strTag, strXML);

  unsigned int i = 0;
  for (i = 0; i < strElement.size(); i++)
  {
    if (i == 0)
    {
      if ((!IsDigit(strElement[i])) && ((strElement[i] != '-')))
        break;
    }
    else
      if (!IsDigit(strElement[i]))
        break;
  }

  // Only try and convert something that is all digits
  if (i == strElement.size())
  {
    if (pFound != NULL)
      *pFound = true;
    return atoi(strElement.c_str());
  }
  else
  {
    if (pFound != NULL)
      *pFound = false;
    return -1;
  }
}

long long XMLUtil::GetElementLongLong(const std::string& strTag, const std::string& strXML, bool* pFound)
{
  std::string strElement = GetElementString(strTag, strXML);

  unsigned int i = 0;
  for (i = 0; i < strElement.size(); i++)
  {
    if (i == 0)
    {
      if ((!IsDigit(strElement[i])) && ((strElement[i] != '-')))
        break;
    }
    else
      if (!IsDigit(strElement[i]))
        break;
  }

  // Only try and convert something that is all digits
  if ((i > 0) && (i == strElement.size()))
  {
    if (pFound != NULL)
      *pFound = true;
    return atoll(strElement.c_str());

  }
  else
  {
    if (pFound != NULL)
      *pFound = false;
    return -1;
  }
}

// Optionally can pass back a bool that tell us if the tag was found
float XMLUtil::GetElementFloat(const std::string& strTag, const std::string& strXML, bool* pFound)
{
  std::string strElement = GetElementString(strTag, strXML);

  bool bFoundDot = false;

  unsigned int i = 0;
  for (i = 0; i < strElement.size(); i++)
  {
    if (i == 0)
    {
      if ((!IsDigit(strElement[i])) && ((strElement[i] != '-')))
        break;
    }
    else
    {
      if (!IsDigit(strElement[i]))
      {
        if ((strElement[i] == '.') && (!bFoundDot))
          bFoundDot = true;
        else
          break;
      }
    }
  }

  // Only try and convert something that is all digits
  if ((i > 0) && (i == strElement.size()))
  {
    if (pFound != NULL)
      *pFound = true;
    return (float) atof(strElement.c_str());
  }
  else
  {
    if (pFound != NULL)
      *pFound = false;
    return (float) 0.0;
  }
}

// Return a vector containing all the text inside all tags matching the passed one
VECTOR_STRING XMLUtil::GetElementStrings(const std::string& strTag, const std::string& strXML, bool bStripWhiteSpace)
{
  VECTOR_STRING vResult;
  vResult.reserve(XML_UTIL_DEFAULT_VECTOR_SIZE);

  std::string strStart = "";
  std::string strEnd = "";
  std::string strResult = "";

  strStart += "<";
  strStart += strTag;
  strStart += ">";

  strEnd += "</";
  strEnd += strTag;
  strEnd += ">";

  size_t iPosStart        = strXML.find(strStart);
  size_t iPosEnd          = strXML.find(strEnd);

  while ((iPosStart != std::string::npos) && (iPosEnd != std::string::npos))
  {
    // We want to be able to handle having the same tag emedded in itself.
    // So between the start tag and the first instance of the end tag,
    // we'll count any other instances of the start tag.  If we find some
    // then we require that we continue until we get that number more of
    // close tags.
    size_t iCurrentStart    = iPosStart + strStart.length();
    size_t iEmbedCount      = 0;
    while ((iCurrentStart != std::string::npos) && (iCurrentStart < iPosEnd))
    {
      iCurrentStart = strXML.find(strStart, iCurrentStart);
      if ((iCurrentStart != std::string::npos) && (iCurrentStart < iPosEnd))
      {
        iEmbedCount++;
        iCurrentStart += strStart.length();
      }
    }
    // Now look for end tag to balance the start tags
    for (size_t i = 0; i < iEmbedCount; i++)
    {
      iPosEnd = strXML.find(strEnd, iPosEnd  + strEnd.length());

      // Check to make sure we're still matching tags
      if (iPosEnd == std::string::npos)
        break;
    }

    strResult = strXML.substr(iPosStart + strStart.length(), iPosEnd - (iPosStart + strStart.length()));

    if (bStripWhiteSpace)
      strResult = StripWhiteSpace(strResult);

    iPosStart = strXML.find(strStart, iPosEnd + strEnd.length());

    if (iPosStart != std::string::npos)
      iPosEnd = strXML.find(strEnd, iPosStart);

    vResult.push_back(strResult);
  }

  return vResult;
}

VECTOR_NAME_VALUE_PAIR XMLUtil::GetNameValuePairs(const std::string& strXML, bool bStripWhiteSpace)
{
  VECTOR_NAME_VALUE_PAIR vResult;
  vResult.reserve(XML_UTIL_DEFAULT_VECTOR_SIZE);

  bool    bInStartTag = false;
  std::string  strName     = "";
  std::string  strValue    = "";

  size_t i = 0;
  while (i < strXML.length())
  {
    if ((!bInStartTag) && (strXML[i] == '<'))
    {
      // Starting a new tag
      bInStartTag = true;
    }
    else if (bInStartTag)
    {
      if (strXML[i] == '>')
      {
        // Hit the end of the start tag, get everything
        // until we find the end tag.

        std::string strFind = "</";
        strFind += strName;
        strFind += ">";

        size_t iPos = std::string::npos;
        iPos = strXML.find(strFind, i);

        if (iPos != std::string::npos)
        {
          strValue = strXML.substr(i + 1, iPos - i - 1);

          NameValuePair sPair;
          sPair.strName    = strName;
          sPair.strValue   = strValue;

          vResult.push_back(sPair);

          i = iPos + strFind.length();

        }
        else
          break;

        bInStartTag     = false;
        strName         = "";
        strValue        = "";
      }
      else
        strName += strXML[i];
    }

    i++;
  }

  return vResult;
}
