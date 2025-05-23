
#pragma once

// Class that stores a particular parameter
// 
// Could be a struct, but we want to sort and need to
// overload the < operator.
//
// Copyright 2005 by Keith Vertanen

#include <string>
#include <vector>
#include "Parameters.h"

// Bit mask options that are sent in when logging parameters
enum eUserLogParam
{
  userLogParamTrackMultiple   = 1,    // Should we track multiple values of a given parameter?
  userLogParamOutputToSimple  = 2,    // Does this parameter get sent to the simple log file?
  userLogParamTrackInTrial    = 4,    // Do we also store a copy of the parameter value within a trial?
  userLogParamForceInTrial    = 8,    // Do we always log the value of this parameter when a new trial is created?
  userLogParamShortInCycle    = 16    // In short logging, does the value get added to the end of a cycle stats line?
};

// We need to have a lookup table that maps parameters we want to track in 
// the UserLog object and what their behavior is.
struct UserLogParamMask {
  Dasher::Parameter key;
  int mask;
};

class CUserLogParam;

typedef std::vector<CUserLogParam*>                      VECTOR_USER_LOG_PARAM_PTR;
typedef std::vector<CUserLogParam*>::iterator            VECTOR_USER_LOG_PARAM_PTR_ITER;
typedef std::vector<CUserLogParam*>::reverse_iterator    VECTOR_USER_LOG_PARAM_PTR_REV_ITER;

/// \ingroup Logging
/// @{
class CUserLogParam
{
public:
  std::string          strName;                // Name of the parameter
  std::string          strValue;               // String version of the value
  std::string          strTimeStamp;           // Optional timestamp if we want to know when a parameter was changed
  int             options;                // The options that were used on the parameter

  static bool     ComparePtr(CUserLogParam* pA, CUserLogParam* pB);
};
/// @}


