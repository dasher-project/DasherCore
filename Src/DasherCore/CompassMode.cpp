// DasherButtons.cpp, build a set of boxes for Button Dasher.
// Copyright 2005, Chris Ball and David MacKay.

// Idea - should back off button always just undo the previous 'forwards' button?

#include "CompassMode.h"

#include <I18n.h>

#include "DasherScreen.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

static SModuleSettings sSettings[] = {
  /* TRANSLATORS: The number of time steps over which to perform the zooming motion in button mode. */
  {LP_ZOOMSTEPS, T_LONG, 1, 63, 1, 1, _("Frames over which to perform zoom")},
  /* TRANSLATORS: The zoom factor per press when moving to the right in compass mode. */
  {LP_RIGHTZOOM, T_LONG, 1024, 10240, 1024, 1024, _("Right zoom")},
  /* TRANSLATORS: Intercept keyboard events for 'special' keys even when the Dasher window doesn't have keyboard focus.*/
  {BP_GLOBAL_KEYBOARD, T_BOOL, -1, -1, -1, -1, _("Global keyboard grab")}
};

// FIX iStyle == 2

CCompassMode::CCompassMode(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface)
    : CDasherButtons(pSettingsStore, pInterface, false /*bMenu*/, _("Compass Mode")), iTargetWidth(0)
{
}

void CCompassMode::SetupBoxes()
{
  m_pBoxes = new SBoxInfo[m_iNumBoxes = 4];

  iTargetWidth = CDasherModel::MAX_Y * 1024 / m_pSettingsStore->GetLongParameter(LP_RIGHTZOOM);

  // FIXME - need to relate these to cross-hair position as stored in the parameters

  // Not sure whether this is at all the right algorithm here - need to check

  m_pBoxes[1].iTop = (2048 - iTargetWidth / 2);
  m_pBoxes[1].iBottom = 4096 - m_pBoxes[1].iTop;

  // Make this the inverse of the right zoom option

  m_pBoxes[3].iTop = -2048 *  m_pBoxes[1].iTop / (2048 -  m_pBoxes[1].iTop);
  m_pBoxes[3].iBottom = 4096 - m_pBoxes[3].iTop;

  m_pBoxes[0].iTop = -iTargetWidth;
  m_pBoxes[0].iBottom = CDasherModel::MAX_Y - iTargetWidth;
  m_pBoxes[2].iTop = iTargetWidth;
  m_pBoxes[2].iBottom = CDasherModel::MAX_Y + iTargetWidth;

  m_pBoxes[0].iDisplayTop = m_pBoxes[0].iTop;
  m_pBoxes[0].iDisplayBottom = m_pBoxes[0].iBottom;
  m_pBoxes[1].iDisplayTop = m_pBoxes[1].iTop;
  m_pBoxes[1].iDisplayBottom = m_pBoxes[1].iBottom;
  m_pBoxes[2].iDisplayTop = m_pBoxes[2].iTop;
  m_pBoxes[2].iDisplayBottom = m_pBoxes[2].iBottom;
  m_pBoxes[3].iDisplayTop = m_pBoxes[3].iTop;
  m_pBoxes[3].iDisplayBottom = m_pBoxes[3].iBottom;
}

bool CCompassMode::DecorateView(CDasherView *pView, CDasherInput *pInput) {
  CDasherScreen *pScreen(pView->Screen());

  int iPos(2048 - iTargetWidth / 2);

  bool bFirst(true);

  while(iPos >= 0) {
    point p[2];

    pView->Dasher2Screen(-100, iPos, p[0].x, p[0].y);

    pView->Dasher2Screen(-1000, iPos, p[1].x, p[1].y);

    if(bFirst)
      pScreen->Polyline(p, 2, 1, pView->GetNamedColor(NamedColor::selectionHighlight));
    else
      pScreen->Polyline(p, 2, 1, pView->GetNamedColor(NamedColor::selectionInactive));

    pView->Dasher2Screen(-100, 4096-iPos, p[0].x, p[0].y);

    pView->Dasher2Screen(-1000, 4096-iPos, p[1].x, p[1].y);

    if(bFirst)
      pScreen->Polyline(p, 2, 1, pView->GetNamedColor(NamedColor::selectionHighlight));
    else
      pScreen->Polyline(p, 2, 1, pView->GetNamedColor(NamedColor::selectionInactive));
     
    iPos -= iTargetWidth;
    bFirst = false;
  }

  bool bRV(m_bDecorationChanged);
  m_bDecorationChanged = false;
  return bRV;
}

void CCompassMode::HandleEvent(Parameter parameter) {
  if (parameter == LP_RIGHTZOOM) {
    delete[] m_pBoxes;
    SetupBoxes();
  }
}

bool CCompassMode::GetSettings(SModuleSettings **pSettings, int *iCount) {
  *pSettings = sSettings;
  *iCount = sizeof(sSettings) / sizeof(SModuleSettings);

  return true;
}
