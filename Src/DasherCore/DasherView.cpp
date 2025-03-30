// DasherView.cpp
//
// Copyright (c) 2008 The Dasher Team
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

#include "DasherInput.h"
#include "DasherView.h"


using namespace Dasher;

/////////////////////////////////////////////////////////////////////////////

CDasherView::CDasherView(CDasherScreen *DasherScreen, Options::ScreenOrientations orient)
 : m_Orientation(orient), m_pScreen(DasherScreen) {
}

/////////////////////////////////////////////////////////////////////////////

void CDasherView::ChangeScreen(CDasherScreen *NewScreen) {
  m_pScreen = NewScreen;
}

/////////////////////////////////////////////////////////////////////////////

void CDasherView::DasherSpaceLine(myint x1, myint y1, myint x2, myint y2, int iWidth, const ColorPalette::Color& color) {
  if (!ClipLineToVisible(x1, y1, x2, y2)) return;
  std::vector<CDasherScreen::point> vPoints;
  CDasherScreen::point p;
  Dasher2Screen(x1, y1, p.x, p.y);
  vPoints.push_back(p);
  DasherLine2Screen(x1,y1,x2,y2,vPoints);
  CDasherScreen::point *pts = new CDasherScreen::point[vPoints.size()];
  for (int i = static_cast<int>(vPoints.size()); i-->0; ) pts[i] = vPoints[i];
  Screen()->Polyline(pts, static_cast<int>(vPoints.size()), iWidth, color);
}

bool CDasherView::ClipLineToVisible(myint &x1, myint &y1, myint &x2, myint &y2) {
  if (x1 > x2) return ClipLineToVisible(x2,y2,x1,y1);
  //ok. have x1 <= x2...
  const CDasherView::DasherCoordScreenRegion vr = VisibleRegion();
  if (x1 > vr.maxX) {
    DASHER_ASSERT(x2>visibleRegion.maxX);
    return false; //entirely offscreen!
  }
  if (x2 < vr.minX) {
    DASHER_ASSERT(x1<visibleRegion.minX);
    return false;
  }
  if (x1 < vr.minX) {
    y1 = y2+((y1-y2)*(vr.minX-x2)/(x1 - x2));
    x1 = vr.minX;
  }
  if (x2 > vr.maxX) {
    y2 = y1 + (y2-y1)*(vr.maxX-x1)/(x2-x1);
    x2 = vr.maxX;
  }
  if (y1 < vr.minY && y2 < vr.minY) return false;
  if (y1 > vr.maxY && y2 > vr.maxY) return false;
  for (int i=0; i<2; i++) {
    myint &y(i ? y2 : y1), &oy(i ? y1 : y2);
    myint &x(i ? x2 : x1), &ox(i ? x1 : x2);
    if (y<vr.minY) {
      x = ox- (ox-x)*(oy-vr.minY)/(oy-y);
      y = vr.minY;
    } else if (y>vr.maxY) {
      x = ox-(ox-x)*(oy-vr.maxY)/(oy-y);
      y = vr.maxY;
    }
  }
  return true;
}

/// Draw a polyline specified in Dasher co-ordinates

void CDasherView::DasherPolyline(myint *x, myint *y, int n, int iWidth, const ColorPalette::Color& color) {

  CDasherScreen::point * ScreenPoints = new CDasherScreen::point[n];

  for(int i(0); i < n; ++i)
    Dasher2Screen(x[i], y[i], ScreenPoints[i].x, ScreenPoints[i].y);

  Screen()->Polyline(ScreenPoints, n, iWidth, color);

  delete[]ScreenPoints;
}

// Draw a polyline with an arrow on the end
void CDasherView::DasherPolyarrow(myint *x, myint *y, int n, int iWidth, const ColorPalette::Color& color, double dArrowSizeFactor) {

  CDasherScreen::point * ScreenPoints = new CDasherScreen::point[n+3];

  for(int i(0); i < n; ++i)
    Dasher2Screen(x[i], y[i], ScreenPoints[i].x, ScreenPoints[i].y);

  int iXvec = (int)((ScreenPoints[n-2].x - ScreenPoints[n-1].x)*dArrowSizeFactor);
  int iYvec = (int)((ScreenPoints[n-2].y - ScreenPoints[n-1].y)*dArrowSizeFactor);

  ScreenPoints[n].x   = ScreenPoints[n-1].x + iXvec + iYvec;
  ScreenPoints[n].y   = ScreenPoints[n-1].y - iXvec + iYvec;
  ScreenPoints[n+1].x = ScreenPoints[n-1].x ;
  ScreenPoints[n+1].y = ScreenPoints[n-1].y ;
  ScreenPoints[n+2].x = ScreenPoints[n-1].x + iXvec - iYvec;
  ScreenPoints[n+2].y = ScreenPoints[n-1].y + iXvec + iYvec;

  Screen()->Polyline(ScreenPoints, n+3, iWidth, color);

  delete[] ScreenPoints;
}

// Draw a box specified in Dasher co-ordinates

void CDasherView::DasherDrawRectangle(myint iDasherMaxX, myint iDasherMinY, myint iDasherMinX, myint iDasherMaxY, const ColorPalette::Color& color, const ColorPalette::Color& outlineColor, int iThickness) {
  //This assertion is more aggressive than necessary (Dasher has been working
  // in many cases where it would fail, with only occassional display glitches)
  // so if it causes trouble, it should be safe to remove...
  DASHER_ASSERT(iDasherMinX <= iDasherMaxX && iDasherMinY <= iDasherMaxY);
  //TODO Parameter names correspond to the values passed in,
  // but the below will only match up with screen coords for LR orientation...
  screenint iScreenLeft;
  screenint iScreenTop;
  screenint iScreenRight;
  screenint iScreenBottom;

  Dasher2Screen(iDasherMaxX, iDasherMinY, iScreenLeft, iScreenTop);
  Dasher2Screen(iDasherMinX, iDasherMaxY, iScreenRight, iScreenBottom);

  Screen()->DrawRectangle(std::min(iScreenLeft, iScreenRight), std::min(iScreenTop, iScreenBottom), std::max(iScreenLeft, iScreenRight), std::max(iScreenTop, iScreenBottom), color, outlineColor, iThickness);
}

/// Draw a rectangle centred on a given dasher co-ordinate, but with a size specified in screen co-ordinates (used for drawing the mouse blob)

void CDasherView::DasherDrawCentredRectangle(myint iDasherX, myint iDasherY, screenint iSize, const ColorPalette::Color& color, const ColorPalette::Color& outlineColor, bool bDrawOutline) {
  screenint iScreenX;
  screenint iScreenY;

  Dasher2Screen(iDasherX, iDasherY, iScreenX, iScreenY);

  Screen()->DrawRectangle(iScreenX - iSize, iScreenY - iSize, iScreenX + iSize, iScreenY + iSize, color, outlineColor, bDrawOutline ? 1 : 0);
}

void CDasherView::SetColorScheme(const ColorPalette* pColorScheme)
{
    m_pColorPalette = pColorScheme;
}

const ColorPalette::Color& CDasherView::GetNamedColor(NamedColor::knownColorName color) const
{
    if(color.empty() || !m_pColorPalette) return ColorPalette::noColor;
    return m_pColorPalette->GetNamedColor(color);
}
