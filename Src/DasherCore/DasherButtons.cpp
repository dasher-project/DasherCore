// DasherButtons.cpp, build a set of boxes for Button Dasher.
// Copyright 2005, Chris Ball and David MacKay.

// Idea - should back off button always just undo the previous 'forwards' button?

#include "DasherButtons.h"
#include "DasherScreen.h"
#include "DasherInterfaceBase.h"
#include <valarray>


using namespace Dasher;

CDasherButtons::CDasherButtons(CSettingsUser *pCreator, CDasherInterfaceBase *pInterface, bool bMenu, ModuleID_t iID, const char *szName)
  : CStaticFilter(pCreator, pInterface, iID, szName), m_bMenu(bMenu), m_bDecorationChanged(true), m_pBoxes(NULL), iActiveBox(0) {}

CDasherButtons::~CDasherButtons()
{
  delete[] m_pBoxes;
} 

void CDasherButtons::Activate() {
  //ick - can't do this at construction time! This should get called before anything
  // which depends on it, tho...
  if (!m_pBoxes) SetupBoxes();

  m_iScanTime = std::numeric_limits<unsigned long>::min();
}

void CDasherButtons::KeyDown(unsigned long iTime, int iId, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {

  if(m_bMenu) {
    switch(iId) {
    case 1:
    case 4:
      m_bDecorationChanged = true;
      ++iActiveBox;
      if(iActiveBox == m_iNumBoxes)
        iActiveBox = 0;
       break;
    case 2:
    case 3:
    case 100:
      m_bDecorationChanged = true;
      ScheduleZoom(pModel, m_pBoxes[iActiveBox].iTop, m_pBoxes[iActiveBox].iBottom);
      if(iActiveBox != m_iNumBoxes-1)
        iActiveBox = 0;
      break;
    }
  }
  else {
    DirectKeyDown(iTime, iId, pView, pModel);
  }

}

void CDasherButtons::DirectKeyDown(unsigned long iTime, int iId, CDasherView *pView, CDasherModel *pModel) {
  if(iId == 100) // Ignore mouse events
  return;
  if(iId == 1)
  iActiveBox = m_iNumBoxes - 1;
  else if(iId <= m_iNumBoxes) 
  iActiveBox = iId-2;
  else
  iActiveBox = m_iNumBoxes-2;

  ScheduleZoom(pModel, m_pBoxes[iActiveBox].iTop,m_pBoxes[iActiveBox].iBottom);
}

void CDasherButtons::Timer(unsigned long Time, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol) {
  if (m_bMenu && GetLongParameter(LP_BUTTON_SCAN_TIME) &&
      Time > m_iScanTime) {
    m_iScanTime = Time + GetLongParameter(LP_BUTTON_SCAN_TIME);
    m_bDecorationChanged = true;
    ++iActiveBox;
    if(iActiveBox == m_iNumBoxes)
      iActiveBox = 0;
  }
  // TODO: This is a bit of a hack to make joystick mode work
  myint iDasherX;
  myint iDasherY;

  pInput->GetDasherCoords(iDasherX, iDasherY, pView);
  // ----
}

void CDasherButtons::NewDrawGoTo(CDasherView *pView, myint iDasherMin, myint iDasherMax, bool bActive) {
   myint iHeight(iDasherMax - iDasherMin);

   CDasherScreen::point p[4];

   pView->Dasher2Screen( 0, iDasherMin, p[0].x, p[0].y);
   pView->Dasher2Screen( iHeight, iDasherMin, p[1].x, p[1].y);
   pView->Dasher2Screen( iHeight, iDasherMax, p[2].x, p[2].y);
   pView->Dasher2Screen( 0, iDasherMax, p[3].x, p[3].y);

   if(bActive) {
     pView->Screen()->Polyline(p, 4, 3, pView->GetColor(ColorPalette::mouseLine));
   }
   else {
     pView->Screen()->Polyline(p, 4, 1, pView->GetColor(ColorPalette::mouseCursor));
   }
}
