



#include "OneButtonDynamicFilter.h"

#include <I18n.h>

#include "DasherInterfaceBase.h"

using namespace Dasher;

static SModuleSettings sSettings[] = {
  /* TRANSLATORS: The time for which a button must be held before it counts as a 'long' (rather than short) press. */
  {LP_HOLD_TIME, T_LONG, 100, 10000, 1000, 100, _("Long press time")},
  /* TRANSLATORS: Double-clicks are special in some situations (they cause us to start reversing). This is the time in which the button must be pressed twice to count.*/
  {LP_MULTIPRESS_TIME, T_LONG, 100, 10000, 1000, 100, _("Double-press time")},
  /* TRANSLATORS: Backoff = reversing in Dasher to correct mistakes. This allows a single button to be dedicated to activating backoff, rather than using multiple presses of other buttons.*/
  {BP_BACKOFF_BUTTON,T_BOOL, -1, -1, -1, -1, _("Enable backoff button")},
  {BP_SLOW_START,T_BOOL, -1, -1, -1, -1, _("Slow startup")},
  {LP_SLOW_START_TIME, T_LONG, 0, 10000, 1000, 100, _("Startup time")},
  {LP_DYNAMIC_SPEED_INC, T_LONG, 1, 100, 1, 1, _("Percentage by which to automatically increase speed")},
  {LP_DYNAMIC_SPEED_FREQ, T_LONG, 1, 1000, 1, 1, _("Time after which to automatically increase speed (secs)")},
  {LP_DYNAMIC_SPEED_DEC, T_LONG, 1, 99, 1, 1, _("Percentage by which to decrease speed upon reverse")}
};

COneButtonDynamicFilter::COneButtonDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate)
  : CButtonMultiPress(pSettingsStore, pInterface, pFramerate, _("One Button Dynamic Mode")) {
  m_iTarget = 0;

  m_iTargetX[0] = 100;
  m_iTargetY[0] = 100;

  m_iTargetX[1] = 100;
  m_iTargetY[1] = 3996; 

  m_bDecorationChanged = true;
}

COneButtonDynamicFilter::~COneButtonDynamicFilter() {
}

bool COneButtonDynamicFilter::DecorateView(CDasherView *pView, CDasherInput *pInput) {

  CDasherScreen *pScreen(pView->Screen());

  screenint x1;
  screenint y1;
  screenint x2;
  screenint y2;

  if(m_iTarget == 1) {
    pView->Dasher2Screen(-100, 0, x1, y1);
    pView->Dasher2Screen(-200, 1000, x2, y2);
  }
  else {
    pView->Dasher2Screen(-100, 3096, x1, y1);
    pView->Dasher2Screen(-200, 4096, x2, y2);
  }

  pScreen->DrawRectangle(x1, y1, x2, y2, ColorPalette::noColor, pView->GetNamedColor(NamedColor::selectionHighlight), 2);
    
  if(m_iTarget == 1) {
    pView->Dasher2Screen(-100, 3096, x1, y1);
    pView->Dasher2Screen(-200, 4096, x2, y2);
  }
  else {
    pView->Dasher2Screen(-100, 0, x1, y1);
    pView->Dasher2Screen(-200, 1000, x2, y2);
  }

  pScreen->DrawRectangle(x1, y1, x2, y2, ColorPalette::noColor, pView->GetNamedColor(NamedColor::selectionInactive), 1);

  bool bRV(m_bDecorationChanged);
  m_bDecorationChanged = false;
  return bRV;
}

void COneButtonDynamicFilter::KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel) {
  if (Key == Keys::Primary_Input && !m_pSettingsStore->GetBoolParameter(BP_BACKOFF_BUTTON))
    //mouse click - will be ignored by superclass method.
    //simulate press of button 2...
    Key= Keys::Button_2;
  CButtonMultiPress::KeyDown(Time, Key, pDasherView, pInput, pModel);
}

void COneButtonDynamicFilter::KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel) {
  if (Key == Keys::Primary_Input && !m_pSettingsStore->GetBoolParameter(BP_BACKOFF_BUTTON))
    //mouse click - will be ignored by superclass method.
    //simulate press of button 2...
    Key= Keys::Button_2;
  CButtonMultiPress::KeyUp(Time, Key, pDasherView, pInput, pModel);
}

void COneButtonDynamicFilter::TimerImpl(unsigned long Time, CDasherView *m_pDasherView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol) {
  OneStepTowards(m_pDasherModel, m_iTargetX[m_iTarget], m_iTargetY[m_iTarget], Time, FrameSpeedMul(m_pDasherModel, Time));
}

void COneButtonDynamicFilter::ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel *pModel) {
  if (iType != 0) {
    //double/long push
    reverse(iTime);
    return;
  }
    
  if((Key == Keys::Button_2) || (Key == Keys::Button_3) || (Key == Keys::Button_4)) {
    if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
      pUserLog->KeyDown(Key, iType, 5);
    m_iTarget = 1 - m_iTarget;
    m_bDecorationChanged = true;
  }
  else {
    if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
      pUserLog->KeyDown(Key, iType, 0);
  }
}


bool COneButtonDynamicFilter::GetSettings(SModuleSettings **pSettings, int *iCount) {
  *pSettings = sSettings;
  *iCount = sizeof(sSettings) / sizeof(SModuleSettings);

  return true;
}
