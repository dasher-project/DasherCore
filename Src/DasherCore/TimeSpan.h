
// Object that keeps track of a time span.
// Span starts when the object is created, and
// ends when someone asks it for its XML representation.
// User can also call stop to cause timer to stop and
// call for XML representation later.
//
// Copyright 2005 by Keith Vertanen

#ifndef __TIME_SPAN_H__
#define __TIME_SPAN_H__

#include "FileLogger.h"
#include <string>
#include "SimpleTimer.h"
#include <vector>


extern CFileLogger* g_pLogger;

class CTimeSpan;

typedef std::vector<CTimeSpan>    VECTOR_TIME_SPAN;
typedef std::vector<CTimeSpan*>   VECTOR_TIME_SPAN_PTR;

/// \ingroup Logging
/// @{
class CTimeSpan
{
public:
  CTimeSpan(const std::string& strName, bool bAddDate);
  CTimeSpan(const std::string& strName, const std::string& strXML);

  ~CTimeSpan();

  void                Stop();
  std::string              GetXML(const std::string& strPrefix = "", bool bSinglePointInTime = false);

  void                Continue();
  bool                IsStopped();
  double              GetElapsed();

  static std::string       GetTimeStamp();
  static std::string       GetDateStamp();

private:
  std::string              m_strName;
  std::string              m_strStartTime;
  std::string              m_strEndTime;
  double              m_dElapsed;
  CSimpleTimer*       m_pTimer;
  std::string              m_strStartDate;

  void                InitMemberVars();

};
/// @}

#endif
