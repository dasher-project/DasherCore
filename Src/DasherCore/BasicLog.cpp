#include "BasicLog.h"

#include "DasherInterfaceBase.h"

#include <cmath>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>

#include "FileUtils.h"


using namespace Dasher;

CBasicLog::CBasicLog(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pIntf)
    : CUserLogBase(pIntf), m_pSettingsStore(pSettingsStore), m_iKeyCount(0), m_iInitialRate(0), m_dBits(0)
{
    m_iSymbolCount = 0;
    m_bStarted = false;
}

CBasicLog::~CBasicLog() {
  EndTrial();
}

void CBasicLog::StartWriting() {
  if(!m_bStarted) {
    StartTrial();
    m_bStarted = true;
  }
}

void CBasicLog::StopWriting(float dNats) {
  m_dBits += dNats / log(2.0);
}

void CBasicLog::AddSymbols(Dasher::VECTOR_SYMBOL_PROB* pVectorNewSymbolProbs, eUserLogEventType iEvent) {
  m_iSymbolCount += static_cast<int>(pVectorNewSymbolProbs->size());
}

void CBasicLog::DeleteSymbols(int iNumToDelete, eUserLogEventType iEvent) {
  m_iSymbolCount -= iNumToDelete;
}

void CBasicLog::NewTrial() {
  EndTrial();
}

void CBasicLog::KeyDown(Dasher::Keys::VirtualKey Key, int iType, int iEffect) {
  ++m_iKeyCount;
}

void CBasicLog::StartTrial() {
  m_iSymbolCount = 0;
  m_iKeyCount = 0;
  m_dBits = 0.0;
  m_strStartDate = GetDateStamp();
  m_iInitialRate = m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE);
}

void CBasicLog::EndTrial() {
  if(!m_bStarted)
    return;

  std::string strFileName(FileUtils::GetFullFilenamePath("dasher_basic.log"));

  std::ofstream oFile;
  oFile.open(strFileName.c_str(), std::ios::out | std::ios::app);

  oFile << "\"" << m_strStartDate << "\":\"" << GetDateStamp() << "\":" << m_iSymbolCount << ":" << m_dBits << ":" << m_iKeyCount << ":" << m_iInitialRate / 100.0 << ":" << m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE) / 100.0 << ":\"" << m_pSettingsStore->GetStringParameter(SP_INPUT_FILTER) << "\":\"" << m_pSettingsStore->GetStringParameter(SP_ALPHABET_ID) << "\"" << std::endl;

  oFile.close();

  m_bStarted = false;
}

std::string CBasicLog::GetDateStamp() {
    auto datestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());  //This is a very long format, should probably be replaced using put_time where its used.
    return std::ctime(&datestamp);	
}
