// TwoButtonDynamicFilter.cpp
//
// Copyright (c) 2007 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software 
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "TwoButtonDynamicFilter.h"

#include <I18n.h>
#include <cmath>

#include "DasherInterfaceBase.h"

using namespace Dasher;

static SModuleSettings sSettings[] = {
  {LP_TWO_BUTTON_OFFSET, T_LONG, 1024, 2048, 2048, 100, _("Button offset")},
  /* TRANSLATORS: The time for which a button must be held before it counts as a 'long' (rather than short) press. */
  {LP_HOLD_TIME, T_LONG, 100, 10000, 1000, 100, _("Long press time")},
  /* TRANSLATORS: Multiple button presses are special (like a generalisation on double clicks) in some situations. This is the maximum time between two presses to count as _part_of_ a multi-press gesture
  (potentially more than two presses). */
  {LP_MULTIPRESS_TIME, T_LONG, 100, 10000, 1000, 100, _("Multiple press interval")},
  /* TRANSLATORS: Backoff = reversing in Dasher to correct mistakes. This allows a single button to be dedicated to activating backoff, rather than using multiple presses of other buttons, and another to be dedicated to starting and stopping. 'Button' in this context is a physical hardware device, not a UI element.*/
  {BP_BACKOFF_BUTTON,T_BOOL, -1, -1, -1, -1, _("Enable backoff and start/stop buttons")},
  /* TRANSLATORS: What is normally the up button becomes the down button etc. */
  {BP_TWOBUTTON_REVERSE,T_BOOL, -1, -1, -1, -1, _("Reverse up and down buttons")},
  /* TRANSLATORS: Pushing the up/down button twice quickly has the same effect as pushing the other
  button once; in this case, one must push three times (or push-and-hold) to reverse. */
  {BP_2B_INVERT_DOUBLE, T_BOOL, -1, -1, -1, -1, _("Double-click is opposite up/down — triple to reverse")},
  {BP_SLOW_START,T_BOOL, -1, -1, -1, -1, _("Slow startup")},
  {LP_SLOW_START_TIME, T_LONG, 0, 10000, 1000, 100, _("Startup time")},
  {LP_DYNAMIC_BUTTON_LAG, T_LONG, 0, 1000, 1, 25, _("Lag before user actually pushes button (ms)")}, 
  {LP_DYNAMIC_SPEED_INC, T_LONG, 1, 100, 1, 1, _("Percentage by which to automatically increase speed")},
  {LP_DYNAMIC_SPEED_FREQ, T_LONG, 1, 1000, 1, 1, _("Time after which to automatically increase speed (secs)")},
  {LP_DYNAMIC_SPEED_DEC, T_LONG, 1, 99, 1, 1, _("Percentage by which to decrease speed upon reverse")}
};

CTwoButtonDynamicFilter::CTwoButtonDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate)
  : CButtonMultiPress(pSettingsStore, pInterface, pFramerate, _("Two Button Dynamic Mode")),
    m_iMouseButton(Keys::Invalid_Key)
{
    CTwoButtonDynamicFilter::ComputeLagBits();

    m_pSettingsStore->OnParameterChanged.Subscribe(this, [this](Parameter parameter)
    {
        switch (parameter) {
          case LP_MAX_BITRATE:// Deliberate fallthrough
          case LP_DYNAMIC_BUTTON_LAG:
              ComputeLagBits();
              m_bDecorationChanged = true;
              break;
          case LP_TWO_BUTTON_OFFSET:
              m_bDecorationChanged = true;
              break;
          default: break;
        }
    });
}

CTwoButtonDynamicFilter::~CTwoButtonDynamicFilter()
{
    m_pSettingsStore->OnParameterChanged.Unsubscribe(this);
}

void CTwoButtonDynamicFilter::ComputeLagBits()
{
    m_dLagBits = m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE)/100.0 * m_pSettingsStore->GetLongParameter(LP_DYNAMIC_BUTTON_LAG)/1000.0;
}

bool CTwoButtonDynamicFilter::DecorateView(CDasherView *pView, CDasherInput *pInput) {
  CDasherScreen *pScreen(pView->Screen());

  point p[2];
  
  myint iDasherX;
  myint iDasherY;
  
  iDasherX = -100;
  iDasherY = 2048 - m_pSettingsStore->GetLongParameter(LP_TWO_BUTTON_OFFSET);
  
  pView->Dasher2Screen(iDasherX, iDasherY, p[0].x, p[0].y);
  
  iDasherX = -1000;
  iDasherY = 2048 - m_pSettingsStore->GetLongParameter(LP_TWO_BUTTON_OFFSET);
  
  pView->Dasher2Screen(iDasherX, iDasherY, p[1].x, p[1].y);
  
  pScreen->Polyline(p, 2, 3, pView->GetNamedColor(NamedColor::selectionHighlight));

  iDasherX = -100;
  iDasherY = 2048 + m_pSettingsStore->GetLongParameter(LP_TWO_BUTTON_OFFSET);
  
  pView->Dasher2Screen(iDasherX, iDasherY, p[0].x, p[0].y);
  
  iDasherX = -1000;
  iDasherY = 2048 + m_pSettingsStore->GetLongParameter(LP_TWO_BUTTON_OFFSET);
  
  pView->Dasher2Screen(iDasherX, iDasherY, p[1].x, p[1].y);
  
  pScreen->Polyline(p, 2, 3, pView->GetNamedColor(NamedColor::selectionHighlight));

  bool bRV(m_bDecorationChanged);
  m_bDecorationChanged = false;
  return bRV;
}

void CTwoButtonDynamicFilter::KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
	if (Key == Keys::Primary_Input && !m_pSettingsStore->GetBoolParameter(BP_BACKOFF_BUTTON)) {
    //mouse click - will be ignored by superclass method.
		//simulate press of button 2/3 according to whether click in top/bottom half
    myint iDasherX, iDasherY;
    m_pInterface->GetActiveInputDevice()->GetDasherCoords(iDasherX, iDasherY, pView);
    m_iMouseButton = Key = (iDasherY < CDasherModel::ORIGIN_Y) ? Keys::Button_2 : Keys::Button_3;
  }
  CButtonMultiPress::KeyDown(Time, Key, pView, pInput, pModel);
}

void CTwoButtonDynamicFilter::KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) {
	if (Key == Keys::Primary_Input && !m_pSettingsStore->GetBoolParameter(BP_BACKOFF_BUTTON)) {
    //mouse click - will be ignored by superclass method.
    //Although we could check current mouse coordinates,
    // since we don't generally do anything in response to KeyUp, seems more consistent just to
    // simulate release of whichever button we depressed in response to mousedown...
    if (m_iMouseButton!= Keys::Invalid_Key) {//paranoia about e.g. pause/resume inbetween press/release?
      Key = m_iMouseButton;
      m_iMouseButton= Keys::Invalid_Key;
    }
  }
  CButtonMultiPress::KeyUp(Time, Key, pView, pInput,pModel);
}

void CTwoButtonDynamicFilter::TimerImpl(unsigned long Time, CDasherView *m_pDasherView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol) {
  OneStepTowards(m_pDasherModel, 100,2048, Time, FrameSpeedMul(m_pDasherModel, Time));
}

void CTwoButtonDynamicFilter::ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel* pModel) {
  
  double dFactor(m_pSettingsStore->GetBoolParameter(BP_TWOBUTTON_REVERSE) ? -1.0 : 1.0);
  int iEffect; //for user log

  if (m_pSettingsStore->GetBoolParameter(BP_2B_INVERT_DOUBLE) && iType == 2 && Key >= Keys::Button_2 && Key <= Keys::Button_4)
  { //double-press - go BACK in opposite direction,
    //far enough to invert previous jump (from first press of double-)
    //and then AGAIN.
    dFactor *= - (1.0 + exp(pModel->GetNats())); //prev jump is further now
  }
  else if (iType != 0) {
    reverse(iTime);
    return;
  }
  
  if(Key == Keys::Button_2) {
    iEffect = 3;
    //fall through to apply offset.
  }
  else if((Key == Keys::Button_3) || (Key == Keys::Button_4)) {
    dFactor = -dFactor;
    iEffect = 4;
    //fall through to apply offset
  }
  else {
    if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
      pUserLog->KeyDown(Key, iType, 0);
    return;
  }
  //fell through to apply offset
  ApplyOffset(pModel, static_cast<int>(dFactor * m_pSettingsStore->GetLongParameter(LP_TWO_BUTTON_OFFSET) * exp(m_dLagBits * FrameSpeedMul(pModel, iTime))));
  pModel->ResetNats();
  
  if(CUserLogBase *pUserLog=m_pInterface->GetUserLogPtr())
    pUserLog->KeyDown(Key, iType, iEffect);  
}

bool CTwoButtonDynamicFilter::GetSettings(SModuleSettings **pSettings, int *iCount) {
  *pSettings = sSettings;
  *iCount = sizeof(sSettings) / sizeof(SModuleSettings);

  return true;
}

bool CTwoButtonDynamicFilter::GetMinWidth(int &iMinWidth) {
  iMinWidth = 1024;
  return true;
}
