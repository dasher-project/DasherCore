
// Handles logging of user activities such as what they write, how 
// long they were writing, mouse positions, parameters, etc
//
// Two different kinds of logging can be produced:
//      1) Running stats file that records basic stats about
//         how much and how fast a user is writing.
//      2) Detailed per session log file for use during
//         user trials.
//
// If detailed mode isn't enabled, calls should stop here in this object
// and not go on to create UserLogTrial objects or anything that isn't
// strictly needed to do the simple logging.
//
// For normal dasher, a new trial involves this sequence:
//      1) Program start or new doc
//      2) Nav start
//      3) Nav stop
//      4) Optionally goto 2
// A new trial must be indicated by the user resetting using new doc event.
// Time from 2-4 is elapsed trial time.
//
// Copyright 2005 by Keith Vertanen

#pragma once

#include "FileLogger.h"
#include <string>
#include <vector>
#include <chrono>
#include "SimpleTimer.h"
#include "TimeSpan.h"
#include "UserLogTrial.h"
#include "UserLogParam.h"
#include "UserLogBase.h"
#include "XMLUtil.h"
#include "SettingsStore.h"

extern CFileLogger* g_pLogger;

const int USER_LOG_DEFAULT_SIZE_TRIAL_XML   = 65536;    // How big we think the XML string representing a single trial will be
const int LOG_MOUSE_EVERY_MS                = 200;      // How often to log the mouse position (-1 for never), the frequency is also depends on how often the WM_TIMER event fires in dasher

static const std::string    USER_LOG_SIMPLE_FILENAME         = "dasher_usage.log";      // Filename of the short text log file
static const std::string    USER_LOG_DETAILED_PREFIX         = "dasher_";               // Prefix of the detailed XML log files
static const bool      USER_LOG_DUMP_AFTER_TRIAL        = true;                    // Do we want to dump the XML after each trial is complete?
static const std::string    USER_LOG_CURRENT_TRIAL_FILENAME  = "CurrentTrial.xml";      // Filename we look for information on what the subject is doing

enum eUserLogLevel
{
  userLogSimple           = 1,    // Simple running log file
  userLogDetailed         = 2     // Detailed per session user trial style
};

#ifndef VECTOR_STRING
typedef std::vector<std::string>                      VECTOR_STRING;
#endif
#ifndef VECTOR_STRING_ITER
typedef std::vector<std::string>::iterator            VECTOR_STRING_ITER;
#endif
#ifndef VECTOR_VECTOR_STRING
typedef std::vector<VECTOR_STRING>               VECTOR_VECTOR_STRING;
#endif
#ifndef VECTOR_VECTOR_STRING_ITER
typedef std::vector<VECTOR_STRING>::iterator     VECTOR_VECTOR_STRING_ITER;
#endif

/// \ingroup Logging
/// @{

// We need to be notified when parameters we are logging get changed, but UserLogBase
// is already watching BP_DASHER_PAUSED
class CUserLog : public CUserLogBase {
public:
  CUserLog(Dasher::CSettingsStore* pSettingsStore, Dasher::CDasherInterfaceBase *pInterface, int iLogTypeMask);

  ~CUserLog() override;

  // Methods called whenever our user interface gets a relevant event, this
  // object will decide how to put it into its representation.
  void AddParam(const std::string& strName, const std::string& strValue, int iOptionMask = 0) override;
  void AddParam(const std::string& strName, double dValue, int iOptionMask = 0) override;
  void AddParam(const std::string& strName, int iValue, int iOptionMask = 0) override;
  void StartWriting() override;
  void StopWriting(float dNats) override;
  void StopWriting() override;
  void AddSymbols(Dasher::VECTOR_SYMBOL_PROB* pVectorNewSymbolProbs, eUserLogEventType iEvent = userLogEventMouse) override;
  void DeleteSymbols(int iNumToDelete, eUserLogEventType iEvent = userLogEventMouse) override;    
  void NewTrial() override;

  void AddWindowSize(int iTop, int iLeft, int iBottom, int iRight) override;
  void AddCanvasSize(int iTop, int iLeft, int iBottom, int iRight) override;
  void AddMouseLocation(int iX, int iY, float dNats) override;
  void AddMouseLocationNormalized(int iX, int iY, bool bStoreIntegerRep, float dNats) override;
  void OutputFile() override;
  void InitIsDone() override;
  void SetOuputFilename(const std::string& strFilename = "") override;
  int  GetLogLevelMask() override;
  void KeyDown(Dasher::Keys::VirtualKey Key, int iType, int iEffect) override;

protected:
  CTimeSpan*								            m_pApplicationSpan;         // How long the application has been up
  std::string									              m_strFilename;              // Name we output our XML file to
  VECTOR_USER_LOG_TRIAL_PTR					    m_vpTrials;                 // Holds object for each trial in this session
  VECTOR_USER_LOG_PARAM_PTR					    m_vParams;                  // Stores general parameters we want in the XML
  std::chrono::steady_clock::time_point	m_dLastMouseUpdate;         // When the last mouse update was pushed
  bool										              m_bSimple;                  // Are we outputting the simple running log file?
  bool										              m_bDetailed;                // Are we outputting per session detailed logs?
  CFileLogger*								          m_pSimpleLogger;            // Used to log the simple running log file
  bool										              m_bIsWriting;               // Has StartWriting() been called but not StopWriting()?
  bool										              m_bInitIsDone;              // Set to true once the initialization of default values is done
  WindowSize								            m_sCanvasCoordinates;       // The size of our canvas from the last call to AddCanvasSize()
  WindowSize								            m_sWindowCoordinates;       // Records the window coordinates at the start of navigation
  bool										              m_bNeedToWriteCanvas;       // Do we need to write new canvas coordinates on the next navigation?
  int										                m_iLevelMask;               // What log level mask we were created with.
  std::string									              m_strCurrentTrialFilename;  // Where info about the current subject's trial is stored

  // Used whenever we need a temporary char* buffer
  static const int            TEMP_BUFFER_SIZE = 4096;
  char                        m_szTempBuffer[TEMP_BUFFER_SIZE];  

  CUserLogTrial*              AddTrial();
  CUserLogTrial*              GetCurrentTrial();
  std::string                      GetXML();
  bool                        WriteXML();
  bool                        UpdateMouseLocation();
  std::string                      GetParamsXML();
  void                        PrepareNewTrial();
  std::string                      GetCycleParamStats();
  std::string                      GetVersionInfo();
  void                        InitMemberVars();
  void                        AddInitialParam();
  void                        UpdateParam(Dasher::Parameter parameter, int iOptionMask);

  // Things that support simple stats of a single Start/Stop cycle:
  Dasher::VECTOR_SYMBOL_PROB  m_vCycleHistory;          // Tracks just the most recent Start/Stop cycle, used for simple logging
  unsigned int                m_iCycleNumDeletes;       // Track number of deletes in last Start/Stop cycle
  CSimpleTimer*               m_pCycleTimer;            // Length of the last Start/Stop cycle
  double                      m_dCycleMouseNormXSum;    // Sum of all normalized mouse X coordinates
  double                      m_dCycleMouseNormYSum;    // Sum of all normalized mouse Y coordinates
  unsigned long               m_iCycleMouseCount;       // How many mouse updates have been stores
  double                      m_dCycleNats;             // The last nats value we got from a mouse event

  std::string                      GetStartStopCycleStats();
  double                      GetCycleBits();
  void                        ComputeSimpleMousePos(int iX, int iY);
  void                        ResetCycle();
  void                        InitUsingMask(int iLogLevelMask);

private:
  Dasher::CSettingsStore* m_pSettingsStore;
};
/// @}

