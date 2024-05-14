// CircleStartHandler.cpp
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

#include "CircleStartHandler.h"
#include "DefaultFilter.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

CCircleStartHandler::CCircleStartHandler(CDefaultFilter *pCreator)
: CStartHandler(pCreator), CSettingsUserObserver(pCreator), m_iEnterTime(std::numeric_limits<long>::max()), m_iScreenRadius(-1), m_pView(nullptr) {
}

CCircleStartHandler::~CCircleStartHandler() {
    if(m_pView) m_pView->OnViewChanged.Unsubscribe(this);
}

CDasherScreen::point CCircleStartHandler::CircleCenter(CDasherView *pView) {
  if (m_iScreenRadius!=-1) return m_screenCircleCenter;

  m_pView->Dasher2Screen(CDasherModel::ORIGIN_X, CDasherModel::ORIGIN_Y, m_screenCircleCenter.x, m_screenCircleCenter.y);
  //compute radius against orientation. It'd be simpler to use
  // Math.min(screen width, screen height) * LP_CIRCLE_PERCENT / 100
  // - should we?
  screenint iEdgeX, iEdgeY;
  m_pView->Dasher2Screen(CDasherModel::ORIGIN_X, CDasherModel::ORIGIN_Y + (CDasherModel::MAX_Y*GetLongParameter(LP_CIRCLE_PERCENT))/100, iEdgeX, iEdgeY);

  const Options::ScreenOrientations iDirection(m_pView->GetOrientation());

  if((iDirection == Options::TopToBottom) || (iDirection == Options::BottomToTop)) {
    m_iScreenRadius = iEdgeX - m_screenCircleCenter.x;
  }
  else {
    m_iScreenRadius = iEdgeY - m_screenCircleCenter.y;
  }
  return m_screenCircleCenter;
}

bool CCircleStartHandler::DecorateView(CDasherView *pView) {
  RegisterView(pView);
  CDasherScreen::point ctr = CircleCenter(pView);

  const bool bAboutToChange = m_bInCircle && m_iEnterTime != std::numeric_limits<long>::max();
  if (m_pFilter->isPaused()) {
    pView->Screen()->DrawCircle(ctr.x, ctr.y, m_iScreenRadius, pView->GetNamedColor(bAboutToChange ? NamedColor::circleWaiting : NamedColor::circleStopped), pView->GetNamedColor(NamedColor::circleOutline), 1);
  } else {
	pView->Screen()->DrawCircle(ctr.x, ctr.y, m_iScreenRadius, ColorPalette::noColor, pView->GetNamedColor(NamedColor::circleOutline), bAboutToChange ? 3 : 1);
  }

  return true;
}

void CCircleStartHandler::Timer(unsigned long iTime, dasherint mouseX, dasherint mouseY,CDasherView *pView) {
  RegisterView(pView);

  CDasherScreen::point ctr = CircleCenter(pView);
  screenint x,y;
  pView->Dasher2Screen(mouseX, mouseY, x, y);
  int dx=x-ctr.x, dy=y-ctr.y;
  const bool inCircleNow = dx*dx + dy*dy <= (m_iScreenRadius * m_iScreenRadius) && pView->Screen()->IsPointVisible(x,y);

  if (inCircleNow) {
    if (m_bInCircle) {
      //still in circle...check they aren't still in there after prev. activation
      if (m_iEnterTime != std::numeric_limits<long>::max() && iTime - m_iEnterTime > 1000) {
        //activate!
        if (m_pFilter->isPaused())
          m_pFilter->run(iTime);
        else
          m_pFilter->stop();
        //note our onPause method will then set
        //   m_iEnterTime = std::numeric_limits<long>::max()
        // thus preventing us from firing until user leaves circle and enters again
      }
    } else {// !m_bInCircle
      //just entered circle
      m_bInCircle=true;
      m_iEnterTime = iTime;
    }
  } else {
    //currently outside circle
    m_bInCircle=false;
  }
}

void CCircleStartHandler::RegisterView(CDasherView *pView)
{
    if(m_pView) return; // We already have one
    m_pView = pView;
    m_pView->OnGeometryChanged.Subscribe(this, [this]()
    {
        m_iScreenRadius = -1;
    });
    m_pView->OnViewChanged.Subscribe(this, [this](CDasherView *pView)
    {
        m_pView->OnGeometryChanged.Unsubscribe(this);
        m_pView->OnViewChanged.Unsubscribe(this);
        RegisterView(pView);
        m_iScreenRadius = -1;
    });
}

void CCircleStartHandler::HandleEvent(Parameter parameter) {
  if (parameter==LP_CIRCLE_PERCENT)
      m_iScreenRadius = -1; //recompute geometry.
}

void CCircleStartHandler::onPause() {
    m_iEnterTime = std::numeric_limits<long>::max();
    //In one-dimensional mode, we have that (un)pausing can _move_ the circle, thus,
    // clicking (or using any other start mechanism) can cause the circle to appear
    // around the mouse. If this happens, you should have to leave and re-enter
    // the circle before the start handler does anything. The following ensures this.
    m_bInCircle = true;
}

void CCircleStartHandler::onRun(unsigned long iTime) {
  //reset things in exactly the same way as when we pause...
  onPause();
}
