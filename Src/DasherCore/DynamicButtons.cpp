// DynamicButtons.cpp



#include "DasherInterfaceBase.h"
#include "DynamicButtons.h"

using namespace Dasher;

CDynamicButtons::CDynamicButtons(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName)
  : CDynamicFilter(pSettingsStore, pInterface, pFramerate, szName), m_pModel(NULL) {
  m_bDecorationChanged = true;
  m_bKeyDown = false;
  pause();
}

void CDynamicButtons::Timer(unsigned long iTime, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol) {
  if (isPaused()) return;
  if (isReversing()) {
    OneStepTowards(pModel, 41943,2048, iTime, FrameSpeedMul(pModel, iTime));
  } else {
    //moving forwards. Check auto speed control...
    if (m_pSettingsStore->GetBoolParameter(BP_AUTO_SPEEDCONTROL) && m_uSpeedControlTime < iTime) {
        if (m_uSpeedControlTime > 0) //has actually been set?
          m_pSettingsStore->SetLongParameter(LP_MAX_BITRATE, static_cast<long>(m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE) * (1.0 + m_pSettingsStore->GetLongParameter(LP_DYNAMIC_SPEED_INC)/100.0)));
        m_uSpeedControlTime = iTime + 1000*m_pSettingsStore->GetLongParameter(LP_DYNAMIC_SPEED_FREQ);
    }
    TimerImpl(iTime, pDasherView, pModel, pol);
  }
}

void CDynamicButtons::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {

  if(((Key == Keys::Big_Start_Stop_Key) || (Key == Keys::Button_1) || (Key == Keys::Primary_Input)) && !m_pSettingsStore->GetBoolParameter(BP_BACKOFF_BUTTON))
    return;

  if(m_bKeyDown)
    return;

  // Pass the basic key down event to the handler
  ButtonEvent(iTime, Key, 0, pModel);

  m_iHeldId = Key;
  m_bKeyDown = true;
}

void CDynamicButtons::KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
  if (Key == m_iHeldId) m_bKeyDown = false;
}

void CDynamicButtons::ButtonEvent(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel *pModel) {
  
  // TODO: Check that state diagram implemented here is what we
  // decided upon

  // What happens next depends on the state:
  if (isPaused()) {
    //Any button causes a restart
    if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
      pUserLog->KeyDown(Key, iType, 1);
    run(iTime);
  } else if (isReversing()) {
    //Any button pauses
    if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
      pUserLog->KeyDown(Key, iType, 2);
    m_pInterface->Done();
    pause();
  } else {
    //running; examine event/button-press type
    switch(iType) {
    case 0: //single press
      if((Key == Keys::Big_Start_Stop_Key) || (Key == Keys::Primary_Input)) {
        //dedicated pause button
        if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
          pUserLog->KeyDown(Key, iType, 2);
        m_pInterface->Done();
        pause();
        break;
      }
      else if(Key == 1) {
        //dedicated reverse button
        if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
          pUserLog->KeyDown(Key, iType, 6);
        reverse(iTime);
        break;
      }
      //else - any non-special button - fall through
    default: //or, Any special kind of event - long, double, triple, ... 
      ActionButton(iTime, Key, iType, pModel);
    }
  }
}

void CDynamicButtons::pause() {
  CDynamicFilter::pause();
  if (m_pModel) m_pModel->AbortOffset();
}

void CDynamicButtons::reverse(unsigned long iTime) {
  m_bForwards=false;
  if (isPaused()) CDynamicFilter::run(iTime);
  if (m_pSettingsStore->GetBoolParameter(BP_AUTO_SPEEDCONTROL)) {
    //treat reversing as a sign of distress --> slow down!
    m_pSettingsStore->SetLongParameter(LP_MAX_BITRATE, static_cast<long>(m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE) *
					 (1.0 - m_pSettingsStore->GetLongParameter(LP_DYNAMIC_SPEED_DEC)/100.0)));
  }
}

void CDynamicButtons::run(unsigned long iTime) {
  m_bForwards=true;
  if (isPaused()) CDynamicFilter::run(iTime); //wasn't running previously
  m_uSpeedControlTime = 0; //will be set in Timer()
}

void CDynamicButtons::ApplyOffset(CDasherModel *pModel, int iOffset) {
  (m_pModel=pModel)->Offset(iOffset);
}
