
// Represent all the data about one trial by a user (ie one
// input sequence using speech or normal Dasher).
//
// Copyright 2005 by Keith Vertanen

#pragma once

#include "FileLogger.h"
#include <string>
#include <vector>
#include "TimeSpan.h"
#include "UserButton.h"
#include "UserLocation.h"
#include "DasherTypes.h"
#include "UserLogParam.h"
#include "XMLUtil.h"

// Types used to return two dimensional grid of double values
typedef double**                                        DENSITY_GRID;
typedef std::vector<DENSITY_GRID>                            VECTOR_DENSITY_GRIDS;
typedef std::vector<DENSITY_GRID>::iterator                  VECTOR_DENSITY_GRIDS_ITER;
typedef std::vector<VECTOR_DENSITY_GRIDS>                    VECTOR_VECTOR_DENSITY_GRIDS;
typedef std::vector<VECTOR_DENSITY_GRIDS>::iterator          VECTOR_VECTOR_DENSITY_GRIDS_ITER;
typedef std::vector<VECTOR_VECTOR_DENSITY_GRIDS>             VECTOR_VECTOR_VECTOR_DENSITY_GRIDS;
typedef std::vector<VECTOR_VECTOR_DENSITY_GRIDS>::iterator   VECTOR_VECTOR_VECTOR_DENSITY_GRIDS_ITER;

extern CFileLogger* g_pLogger;

class CUserLogTrial;

typedef std::vector<CUserLogTrial>               VECTOR_USER_LOG_TRIAL;
typedef std::vector<CUserLogTrial>::iterator     VECTOR_USER_LOG_TRIAL_ITER;
typedef std::vector<CUserLogTrial*>              VECTOR_USER_LOG_TRIAL_PTR;
typedef std::vector<CUserLogTrial*>::iterator    VECTOR_USER_LOG_TRIAL_PTR_ITER;

// Used to indicate what type of event caused symbols to
// be added or deleted by Dasher.  This could for example
// be used to log which button was pressed in button
// Dasher.
enum eUserLogEventType
{
  userLogEventMouse       = 0     // Normal mouse navigation
};

// Keeps track of a single instance of AddSymbols() or 
// DeleteSymbols() being called.  
struct NavLocation
{
  std::string                                  strHistory;     // Display symbol history after the adds or deletes are carried out
  CTimeSpan*                              span;           // Track the time between this update and the one that comes next
  eUserLogEventType                       event;          // What triggered the adding or deleting of symbols
  int                                     numDeleted;     // How many symbols deleted (0 if it is an AddSymbols() call)
  Dasher::VECTOR_SYMBOL_PROB*             pVectorAdded;   // Info on all added symbols   
  double                                  avgBits;        // Average bits required to write this history (assuming no errors)
};

typedef std::vector<NavLocation*>    VECTOR_NAV_LOCATION_PTR;

struct WindowSize
{
  int         top;
  int         left;
  int         bottom;
  int         right;
};

// Stores the time span and all the locations and mouse locations
// that occur during a start/stop cycle of navigation.
struct NavCycle
{
  CTimeSpan*                  pSpan;                    // Tracks time span of this navgiation cycle
  VECTOR_NAV_LOCATION_PTR     vectorNavLocations;       // Locations when text was added or deleted
  VECTOR_USER_LOCATION_PTR    vectorMouseLocations;     // Stores mouse locations and time stamps    
  VECTOR_USER_BUTTON_PTR      vectorButtons;        // Stores button presses and time stamps
  double dBits; // Number of bits entered during the cycle, only updated at the end
};

typedef std::vector<NavCycle*>               VECTOR_NAV_CYCLE_PTR;
typedef std::vector<NavCycle*>::iterator     VECTOR_NAV_CYCLE_PTR_ITER;

/// \ingroup Logging
/// @{
class CUserLogTrial
{
public:
  CUserLogTrial(const std::string& strCurrentTrialFilename);
  ~CUserLogTrial();

  bool                        HasWritingOccured();
  void                        StartWriting();
  void                        StopWriting(double dBits);
  void                        AddSymbols(Dasher::VECTOR_SYMBOL_PROB* vpNewSymbolProbs, eUserLogEventType iEvent);
  void                        DeleteSymbols(int iNumToDelete, eUserLogEventType iEvent);  
  std::string                      GetXML(const std::string& strPrefix = "");
  void                        Done();
  void                        AddWindowSize(int iTop, int iLeft, int iBottom, int iRight);
  void                        AddCanvasSize(int iTop, int iLeft, int iBottom, int iRight);
  void                        AddMouseLocation(int iX, int iY, float dNats);
  void                        AddMouseLocationNormalized(int iX, int iY, bool bStoreIntegerRep, float dNats);
  void AddKeyDown(Dasher::Keys::VirtualKey Key, int iType, int iEffect);
  bool                        IsWriting();
  void                        AddParam(const std::string& strName, const std::string& strValue, int iOptionMask = 0);
  static std::string               GetParamXML(CUserLogParam* pParam, const std::string& strPrefix = "");

  int GetButtonCount();
  double GetTotalBits();

  // Methods used by utility that can post-process the log files:
  CUserLogTrial(const std::string& strXML, int iIgnored);
  static VECTOR_USER_LOG_PARAM_PTR    ParseParamsXML(const std::string& strXML);
  static WindowSize                   ParseWindowXML(const std::string& strXML);
  VECTOR_STRING                       GetTabMouseXY(bool bReturnNormalized);
  VECTOR_DENSITY_GRIDS                GetMouseDensity(int iGridSize);
  static DENSITY_GRID                 MergeGrids(int iGridSize, DENSITY_GRID pGridA, DENSITY_GRID pGridB);

protected:
  CTimeSpan*                          m_pSpan;
  bool                                m_bWritingStart;
  std::string                              m_strCurrentTrial;          // Stores information passed to us from the UserTrial app
  WindowSize                          m_sWindowCoordinates;       // Records the window coordinates at the start of navigation
  WindowSize                          m_sCanvasCoordinates;       // The size of our canvas during navigation
  Dasher::VECTOR_SYMBOL_PROB          m_vHistory;                 // Tracks all the symbols, probs, display text entererd during this trial
  VECTOR_USER_LOG_PARAM_PTR           m_vpParams;                 // Stores general parameters we want stored in each trial tag in the XML
  VECTOR_NAV_CYCLE_PTR                m_vpNavCycles;
  std::string                              m_strCurrentTrialFilename;  // Where to look for info on the current subject's trial

  // Used whenever we need a temporary char* buffer
  static const int                    TEMP_BUFFER_SIZE = 4096;
  char                                m_szTempBuffer[TEMP_BUFFER_SIZE];  

  void                        GetUserTrialInfo();
  std::string                      GetHistoryDisplay();
  double                      GetHistoryAvgBits();
  void                        StopPreviousTimer();
  void                        InitMemberVars();

  NavCycle*                   GetCurrentNavCycle();
  NavCycle*                   AddNavCycle();
  NavLocation*                GetCurrentNavLocation();

  // Various helpers for outputting the XML, this allows subclasses to
  // add there own GetXML() method but reuse code for shared parts.
  std::string                      GetLocationXML(NavLocation* pLocation, const std::string& strPrefix);
  std::string                      GetSummaryXML(const std::string& strPrefix);
  std::string                      GetStatsXML(const std::string& strPrefix, const std::string& strText, CTimeSpan* pSpan, double dAvgBits, int iButtonCount, double dTotalBits);
  std::string                      GetWindowCanvasXML(const std::string& strPrefix);
  std::string                      GetParamsXML(const std::string& strPrefix);
  std::string                      GetNavCyclesXML(const std::string& strPrefix);

};
/// @}

