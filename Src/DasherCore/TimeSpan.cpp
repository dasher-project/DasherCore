#include <cstring>
#include <chrono>
#include <ctime>
#include "TimeSpan.h"

#include "XMLUtil.h"


CTimeSpan::CTimeSpan(const std::string& strName, bool bAddDate)
{
  InitMemberVars();

  m_strName       = strName;
  m_strStartTime  = GetTimeStamp();
  m_pTimer        = new CSimpleTimer();
  m_dElapsed      = 0.0;
  m_strEndTime    = "";
  m_strStartDate  = "";

  // A time span can optionally record the current date
  if (bAddDate)
    m_strStartDate = GetDateStamp();
}

CTimeSpan::~CTimeSpan()
{
  if (m_pTimer != NULL)
  {
    delete m_pTimer;
    m_pTimer = NULL;
  }
}

// Get the XML for this TimeSpan object.  If bSinglePointInTime is true, then 
// this is just a single point in time and we don't need the end time or 
// elapsed time.
std::string CTimeSpan::GetXML(const std::string& strPrefix, bool bSinglePointInTime)
{
  std::string strResult;

  // Only stop if we haven't called Stop() explicitly
  if (m_strEndTime.empty())
    Stop();

  strResult += strPrefix;
  strResult += "<";
  strResult += m_strName;
  strResult += ">\n";

  if (!bSinglePointInTime)
  {
    strResult += strPrefix;
    strResult += "\t<Elapsed>";
    char strNum[256];
    sprintf(strNum, "%0.3f", m_dElapsed);
    strResult += strNum;
    strResult += "</Elapsed>\n";
  }

  if (m_strStartDate.length() > 0)
  {
    strResult += strPrefix;
    strResult += "\t<Date>";
    strResult += m_strStartDate;
    strResult += "</Date>\n";
  }

  if (!bSinglePointInTime)
  {
    strResult += strPrefix;
    strResult += "\t<Start>";
    strResult += m_strStartTime;
    strResult += "</Start>\n";

    strResult += strPrefix;
    strResult += "\t<End>";
    strResult += m_strEndTime;
    strResult += "</End>\n";
  }
  else
  {
    strResult += strPrefix;
    strResult += "\t<Time>";
    strResult += m_strStartTime;
    strResult += "</Time>\n";
  }

  strResult += strPrefix;
  strResult += "</";
  strResult += m_strName;
  strResult += ">\n";

  return strResult;
}

std::string CTimeSpan::GetTimeStamp()
{
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm time_tm = *std::localtime(&time_t);
  char buf[80];
  std::strftime(buf, sizeof(buf), "%H:%M:%S", &time_tm);

  return std::string(buf) + "." + std::to_string(ms.count());
}

void CTimeSpan::Stop()
{
  // Only do this if we actually have a timer, if we were
  // created from XML then we don't want to change what 
  // we read from the file.
  if (m_pTimer != NULL)
  {
    m_strEndTime    = GetTimeStamp();
    m_dElapsed       = m_pTimer->GetElapsed();
  }
}

// We allow a time span to continue to erase the 
// effects of a previous Stop() call
void CTimeSpan::Continue()
{
  m_strEndTime = "";
  m_dElapsed = 0.0;
}

bool CTimeSpan::IsStopped()
{
  if (m_strEndTime.length() > 0)
    return true;

  return false;
}

double CTimeSpan::GetElapsed()
{
  return m_dElapsed;
}

std::string CTimeSpan::GetDateStamp()
{
  std::string strDateStamp = "";
  char* szTimeLine = NULL;
  time_t t;

  t = time(NULL);
  szTimeLine = ctime(&t);

  // Format is:
  // Wed Jun 22 10:22:00 2005
  // 0123456789012345678901234
  if ((szTimeLine != NULL) && (strlen(szTimeLine) > 23))
  {
    for (int i = 4; i < 10; i++)
      strDateStamp += szTimeLine[i];            
    for (int i = 19; i < 24; i++)
      strDateStamp += szTimeLine[i];            
  }

  return strDateStamp;
}

void CTimeSpan::InitMemberVars()
{
  m_pTimer        = NULL;
  m_strName       = "";
  m_strStartTime  = "";
  m_dElapsed      = 0.0;
  m_strEndTime    = "";
  m_strStartDate  = "";
}

// Construct based on some yummy XML like:
// 		<Elapsed>12.062</Elapsed>
//		<Date>Jul 04 2005</Date>
//		<Start>15:48:52.625</Start>
//		<End>15:49:04.687</End>
CTimeSpan::CTimeSpan(const std::string& strName, const std::string& strXML)
{
  InitMemberVars();

  m_dElapsed      = (double) XMLUtil::GetElementFloat("Elapsed", strXML);    

  m_strStartDate  = XMLUtil::GetElementString("Date", strXML);
  m_strStartTime  = XMLUtil::GetElementString("Start", strXML);
  m_strEndTime    = XMLUtil::GetElementString("End", strXML);

  m_strName       = strName;
}
