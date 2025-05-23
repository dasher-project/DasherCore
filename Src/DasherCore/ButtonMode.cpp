// DasherButtons.cpp, build a set of boxes for Button Dasher.
// Copyright 2005, Chris Ball and David MacKay.
// Idea - should back off button always just undo the previous 'forwards' button?

#include "ButtonMode.h"

#include <I18n.h>
#include <cmath>

#include "DasherScreen.h"
#include "DasherInterfaceBase.h"


using namespace Dasher;

static SModuleSettings sSettings[] = {
  /* TRANSLATORS: The number of time steps over which to perform the zooming motion in button mode. */
  {LP_ZOOMSTEPS, T_LONG, 1, 63, 1, 1, _("Frames over which to perform zoom")},
  {LP_BUTTON_SCAN_TIME, T_LONG, 0, 2000, 1, 100, _("Scan time in menu mode (0 to not scan)")},
  {LP_B, T_LONG, 2, 10, 1, 1, _("Number of forward boxes")},
  {LP_S, T_LONG, 0, 256, 1, 1, _("Safety margin")},
  /* TRANSLATORS: The boxes (zoom targets) in button mode can either be the same size, or different sizes - this is the extent to which the sizes are allowed to differ from each other. */
  /* XXX PRLW: 128 log(2) = 89, where 2 is the ratio of adjacent boxes
   * however the code seems to use ratio = (129/127)^-r, instead of
   * ratio = exp(r/128) used in the design document
   */
  {LP_R, T_LONG, -89, 89, 1, 10, _("Box non-uniformity")},
  /* TRANSLATORS: Intercept keyboard events for 'special' keys even when the Dasher window doesn't have keyboard focus.*/
  {BP_GLOBAL_KEYBOARD, T_BOOL, -1, -1, -1, -1, _("Global keyboard grab")}
};

// FIX iStyle == 0

CButtonMode::CButtonMode(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, bool bMenu, const char *szName)
: CDasherButtons(pSettingsStore, pInterface, bMenu, szName)
{
    m_pSettingsStore->OnParameterChanged.Subscribe(this, [this](Parameter p)
    {
        switch (p) {
          case LP_B:
          case LP_R:
            // Deliberate fallthrough
            delete[] m_pBoxes;
            SetupBoxes();
            m_pInterface->ScheduleRedraw();
            break;
          default: break;
        }
    });
}

CButtonMode::~CButtonMode()
{
    m_pSettingsStore->OnParameterChanged.Unsubscribe(this);
}

void CButtonMode::SetupBoxes()
{
  int iDasherY(CDasherModel::MAX_Y);

  int iForwardBoxes(m_pSettingsStore->GetLongParameter(LP_B));
  m_pBoxes = new SBoxInfo[m_iNumBoxes = iForwardBoxes+1];

  // Calculate the sizes of non-uniform boxes using standard
  // geometric progression results

  // FIXME - implement this using DJCM's integer method?
  // See ~mackay/dasher/buttons/
  const double dRatio = pow(129/127.0, -static_cast<double>(m_pSettingsStore->GetLongParameter(LP_R)));

  if(m_bMenu) {

    double dMaxSize;
    if(dRatio == 1.0)
      dMaxSize = iDasherY / static_cast<double>(iForwardBoxes);
    else
      dMaxSize = ((dRatio - 1)/(pow(dRatio, iForwardBoxes) - 1)) * iDasherY;

    double dMin(0.0);
    double dMax;

    for(int i(0); i < m_iNumBoxes - 1; ++i) { // One button reserved for backoff
      dMax = dMin + dMaxSize * pow(dRatio, i);

//       m_pBoxes[i].iDisplayTop = (i * iDasherY) / (m_iNumBoxes - 1);
//       m_pBoxes[i].iDisplayBottom = ((i+1) * iDasherY) / (m_iNumBoxes - 1);

      m_pBoxes[i].iDisplayTop = static_cast<int>(dMin);
      m_pBoxes[i].iDisplayBottom = static_cast<int>(dMax);

      m_pBoxes[i].iTop = m_pBoxes[i].iDisplayTop - m_pSettingsStore->GetLongParameter(LP_S);
      m_pBoxes[i].iBottom = m_pBoxes[i].iDisplayBottom + m_pSettingsStore->GetLongParameter(LP_S);

      dMin = dMax;
    }

  }
  else {
    if(iForwardBoxes == 2) { // Special case for two forwards buttons
      myint iMid = static_cast<int>(iDasherY / (1.0+dRatio));

      m_pBoxes[0].iDisplayTop = 0;
      m_pBoxes[0].iDisplayBottom = static_cast<int>(iMid);

      m_pBoxes[1].iDisplayTop = static_cast<int>(iMid);
      m_pBoxes[1].iDisplayBottom = iDasherY;
    }
    else {
      bool bEven(iForwardBoxes % 2 == 0);

      const int iGeometricTerms = (iForwardBoxes+1)/2; //int div, round down

      double dMaxSize;

      if(dRatio == 1.0) {
        dMaxSize = static_cast<double>(iDasherY) / static_cast<double>(iForwardBoxes);
      }
      else {
        if(bEven)
          dMaxSize = iDasherY * (dRatio - 1) / (2 * (pow(dRatio, iGeometricTerms) - 1));
        else
          dMaxSize = iDasherY * (dRatio - 1) / (2 * (pow(dRatio, iGeometricTerms) - 1) - (dRatio - 1));
      }

      double dMin = (bEven) ? iDasherY/2.0 : (iDasherY-dMaxSize)/2.0;

      const int iUpBase = iForwardBoxes/2; //int div, round down if !bEven
      const int iDownBase = bEven ? iUpBase-1 : iUpBase;


      for(int i(0); i < iGeometricTerms; ++i) { // One button reserved for backoff
        const double dMax = dMin + dMaxSize * pow(dRatio, i);

        m_pBoxes[iUpBase + i].iDisplayTop = int(dMin);
        m_pBoxes[iUpBase + i].iDisplayBottom = int(dMax);

        m_pBoxes[iDownBase - i].iDisplayTop = int(iDasherY - dMax);
        m_pBoxes[iDownBase - i].iDisplayBottom = int(iDasherY - dMin);

        dMin = dMax;
      }
    }
  }

  for(int i(0); i < m_iNumBoxes - 1; ++i) {
    m_pBoxes[i].iTop = m_pBoxes[i].iDisplayTop - m_pSettingsStore->GetLongParameter(LP_S);
    m_pBoxes[i].iBottom = m_pBoxes[i].iDisplayBottom + m_pSettingsStore->GetLongParameter(LP_S);
  }

  m_pBoxes[m_iNumBoxes-1].iDisplayTop = 0;
  m_pBoxes[m_iNumBoxes-1].iDisplayBottom = iDasherY;

  m_pBoxes[m_iNumBoxes-1].iTop = int(- iDasherY / 2);
  m_pBoxes[m_iNumBoxes-1].iBottom = int(iDasherY * 1.5);
}

bool CButtonMode::DecorateView(CDasherView *pView, CDasherInput *pInput) {
  for(int i(0); i < m_iNumBoxes; ++i) {
    if(i != iActiveBox)
      NewDrawGoTo(pView, m_pBoxes[i].iDisplayTop, m_pBoxes[i].iDisplayBottom, false);
  }
  NewDrawGoTo(pView, m_pBoxes[iActiveBox].iDisplayTop, m_pBoxes[iActiveBox].iDisplayBottom, m_bMenu || m_bHighlight);

  bool bRV(m_bDecorationChanged);
  m_bDecorationChanged = false;
  return bRV;
}

void CButtonMode::Timer(unsigned long Time, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol) {
  bool m_bOldHighlight(m_bHighlight);
  m_bHighlight = (Time - m_iLastTime < 200);

  if(m_bOldHighlight != m_bHighlight)
    m_bDecorationChanged = true;

  CDasherButtons::Timer(Time, pView, pInput, pModel, pol);
}

void CButtonMode::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
  if (Key == Keys::Primary_Input) {
    //Mouse!
    if (m_bMenu) {
      bool bScan;
      if (m_pSettingsStore->GetLongParameter(LP_BUTTON_SCAN_TIME))
        bScan = false; //auto-scan, any click selects
      else {
        //top scans, bottom selects
        screenint iScreenX, iScreenY;
        pInput->GetScreenCoords(iScreenX, iScreenY, pView);
        bScan = iScreenY < pView->Screen()->GetHeight()/2;
      }
      CDasherButtons::KeyDown(iTime, bScan ? Keys::Button_1 : Keys::Button_2, pView, pInput, pModel);
      return;
    } else {
      myint iDasherX, iDasherY;
      pInput->GetDasherCoords(iDasherX, iDasherY, pView);
      //look for a click _in_ a box -> activate box
      for (int i = 0; i < m_iNumBoxes; i++) {
        if (iDasherY < m_pBoxes[i].iDisplayBottom &&
            iDasherY > m_pBoxes[i].iDisplayTop &&
            iDasherX < (m_pBoxes[i].iDisplayBottom - m_pBoxes[i].iDisplayTop)) {
          //user has clicked in box! Simulate press of appropriate (direct-mode) button...
          CDasherButtons::KeyDown(iTime, (i==m_iNumBoxes-1) ? Keys::Button_1 : static_cast<Keys::VirtualKey>(i+2), pView, pInput, pModel);
          return;
        }
      }
      //not in any box. Fall through, just to be conservative...
    }
  }
  CDasherButtons::KeyDown(iTime, Key, pView, pInput, pModel);
}

void CButtonMode::DirectKeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherModel *pModel) {
  CDasherButtons::DirectKeyDown(iTime, Key, pView, pModel);
 if (Key!=Keys::Primary_Input) m_iLastTime = iTime;
}

bool CButtonMode::GetSettings(SModuleSettings **pSettings, int *iCount) {
  *pSettings = sSettings;
  *iCount = sizeof(sSettings) / sizeof(SModuleSettings);

  return true;
}
