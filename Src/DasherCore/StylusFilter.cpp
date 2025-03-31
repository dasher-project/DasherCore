#include "StylusFilter.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

static SModuleSettings sSettings[] = {
  {LP_TAP_TIME, T_LONG, 1, 1000, 1, 25, _("Max time for a 'tap' (anything longer is held)")},
  {LP_ZOOMSTEPS, T_LONG, 1, 63, 1, 1, _("Frames over which to perform zoom")},
};

CStylusFilter::CStylusFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName)
  : CDefaultFilter(pSettingsStore, pInterface, pFramerate, szName), m_pModel(NULL) {
}

void CStylusFilter::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
  if(Key == Keys::Primary_Input) {
    //pModel->ClearScheduledSteps(); //no need - each one step scheduled by superclass, will do this
    run(iTime);
    m_iKeyDownTime = iTime;
  } else
    CDefaultFilter::KeyDown(iTime, Key, pView, pInput, pModel);
}

void CStylusFilter::pause() {
  CDefaultFilter::pause();
  if (m_pModel) m_pModel->ClearScheduledSteps();
}

void CStylusFilter::KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
  if(Key == Keys::Primary_Input) {
    pause(); //stops superclass from scheduling any more one-step movements
    if (iTime - m_iKeyDownTime < static_cast<unsigned long>(m_pSettingsStore->GetLongParameter(LP_TAP_TIME))) {
      pInput->GetDasherCoords(m_iLastX, m_iLastY, pView);
      ApplyClickTransform(m_iLastX, m_iLastY, pView);
      (m_pModel=pModel)->ScheduleZoom(m_iLastY-m_iLastX, m_iLastY+m_iLastX, m_pSettingsStore->GetLongParameter(LP_ZOOMSTEPS));
    } else {
      m_pInterface->Done();
    }
  } else
    CDefaultFilter::KeyUp(iTime, Key, pView, pInput, pModel);
}

void CStylusFilter::ApplyClickTransform(myint &iDasherX, myint &iDasherY, CDasherView *pView) {
  AdjustZoomX(iDasherX, pView, m_pSettingsStore->GetLongParameter(LP_S), m_pSettingsStore->GetLongParameter(LP_MAXZOOM));
}

CStartHandler *CStylusFilter::MakeStartHandler() {
  return NULL;
}

bool CStylusFilter::GetSettings(SModuleSettings **pSettings, int *iCount) {
  *pSettings = sSettings;
  *iCount = sizeof(sSettings) / sizeof(sSettings[0]);
  return true;
}
