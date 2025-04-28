// DasherViewSquare.cpp
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

#include "DasherViewSquare.h"
#include "DasherView.h"
#include "DasherTypes.h"
#include "Event.h"

#include <algorithm>
#include <limits>
#include <cmath>

using namespace Dasher;

// FIXME - quite a lot of the code here probably should be moved to
// the parent class (DasherView). I think we really should make the
// parent class less general - we're probably not going to implement
// anything which uses radically different co-ordinate transforms, and
// we can always override if necessary.

/////////////////////////////////////////////////////////////////////////////
// Functions reimplemented from description


/////////////////////////////////////////////////////////////////////////////

CDasherViewSquare::CDasherViewSquare(CSettingsStore* pSettingsStore, CDasherScreen* DasherScreen, Options::ScreenOrientations orient)
	: CDasherView(DasherScreen, orient), m_pSettingsStore(pSettingsStore)
{
	//Note, nonlinearity parameters set in SetScaleFactor
	ScreenResized(DasherScreen);

	m_pSettingsStore->OnParameterChanged.Subscribe(this, [this](const Parameter parameter)
    {
        if (parameter == LP_MARGIN_WIDTH ||
		    parameter == BP_NONLINEAR_Y ||
		    parameter == LP_NONLINEAR_X ||
		    parameter == LP_GEOMETRY)
	    {
		    m_bVisibleRegionValid = false;
		    SetScaleFactor();
	    }
    });
}

CDasherViewSquare::~CDasherViewSquare()
{
	m_pSettingsStore->OnParameterChanged.Unsubscribe(this);
}

void CDasherViewSquare::SetOrientation(Options::ScreenOrientations newOrient)
{
	if (newOrient == GetOrientation()) return;
	CDasherView::SetOrientation(newOrient);
	m_bVisibleRegionValid = false;
	SetScaleFactor();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////


CDasherNode* CDasherViewSquare::Render(CDasherNode* pRoot, myint iRootMin, myint iRootMax,
                                       CExpansionPolicy& policy)
{
	DASHER_ASSERT(pRoot != 0);
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();
	const ScreenRegion screenRegion = {0,0,Screen()->GetWidth(), Screen()->GetHeight()};

	CDasherNode* currentTopCenterNode = pRoot->Parent(); //Node under crosshair

	// Blank the region around the root node:
	if (m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE) == Options::DISJOINT_RECTANGLE)
	{
		//disjoint rects, so go round root
		if (iRootMin > visibleRegion.minY)
			DasherDrawRectangle(visibleRegion.maxX, visibleRegion.minY, visibleRegion.minX, iRootMin, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);

		if (iRootMax < visibleRegion.maxY)
			DasherDrawRectangle(visibleRegion.maxX, iRootMax, visibleRegion.minX, visibleRegion.maxY, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);

		//to left (greater Dasher X)
		if (iRootMax - iRootMin < visibleRegion.maxX)
			DasherDrawRectangle(visibleRegion.maxX, std::max(iRootMin, visibleRegion.minY), iRootMax - iRootMin, std::min(iRootMax, visibleRegion.maxY), GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);

		//to right (margin)
		DasherDrawRectangle(0, visibleRegion.minY, visibleRegion.minX, visibleRegion.maxY, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);

		//and render root.
		DisjointRender(pRoot, iRootMin, iRootMax, nullptr, policy, std::numeric_limits<double>::infinity(), currentTopCenterNode);
	}
	else if(m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE) == Options::CUBE)
	{
		//Render white box to left side of screen if other nodes do not completely cover the screen
		if(IsSpaceAroundNode(iRootMin, iRootMax))
			DasherDrawCube(visibleRegion.maxX, visibleRegion.minY, 0, visibleRegion.maxY, {-1,0}, {-1,-1}, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0, nullptr);

		m_CrosshairCubeLevel = -1;
		NewRender(pRoot, iRootMin, iRootMax, nullptr, policy, std::numeric_limits<double>::infinity(), currentTopCenterNode, {0,0},{-1,0}, screenRegion);

		//to right (margin)
		DasherDrawCube(0, visibleRegion.minY, visibleRegion.minX, visibleRegion.maxY, {m_CrosshairCubeLevel,0}, {-1,-1}, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0, nullptr);

		// Get Origin
		screenint originX, originY = -1;
		Dasher2Screen(CDasherModel::ORIGIN_X, CDasherModel::ORIGIN_Y, originX, originY);

		//Vertical Bar of Crosshair
		screenint sizeX = 7, sizeY = static_cast<screenint>(static_cast<float>(Screen()->GetHeight()) * 1.05f);
		if(GetOrientation() == Options::TopToBottom || GetOrientation() == Options::BottomToTop) std::swap(sizeX, sizeY);
		Screen()->DrawProjectedRectangle(originX, originY, sizeX, sizeY);

		// Print 3DText
		for (geometry_3DText& text : m_Delayed3DTexts)
		{
			DoDelayedText(text.root_node, text.extrusionLevel, text.groupRecursionDepth);
		}
		m_Delayed3DTexts.clear();

		//Backshift all cubes and letters
		Screen()->FinishRender3D(originX, originY, m_CrosshairCubeLevel);

		// Finally decorate the view
		// Crosshair();
		return currentTopCenterNode;
	}
	else
	{
		//overlapping rects/shapes
		if (currentTopCenterNode)
		{
			//LEFT of Y axis, would be entirely covered by the root node parent (before we render root)
			// (getColor() gives the right color, even if pOutput is invisible - in that case it gives
			// the color of its parent)
			DasherDrawRectangle(visibleRegion.maxX, visibleRegion.minY, 0, visibleRegion.maxY, currentTopCenterNode->getNodeColor(m_pColorPalette), GetNamedColor(NamedColor::defaultOutline), 0);
			//RIGHT of Y axis, should be white.
			DasherDrawRectangle(0, visibleRegion.minY, visibleRegion.minX, visibleRegion.maxY, GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);
		}
		else //easy case, whole screen is white (outside root node, e.g. when starting)
		{
			Screen()->DrawRectangle(0, 0, Screen()->GetWidth(), Screen()->GetHeight(), GetNamedColor(NamedColor::background), GetNamedColor(NamedColor::defaultOutline), 0);
		}
		NewRender(pRoot, iRootMin, iRootMax, nullptr, policy, std::numeric_limits<double>::infinity(), currentTopCenterNode, {0,0},{0,0}, screenRegion);
	}

	// Labels are drawn in a second parse to get the overlapping right
	for (auto& m_DelayedText : m_DelayedTexts)
		DoDelayedText(m_DelayedText);
	m_DelayedTexts.clear();

	// Finally decorate the view
	Crosshair();
	return currentTopCenterNode;
}

/// Draw text specified in Dasher co-ordinates. The position is
/// specified as two co-ordinates, intended to the be the corners of
/// the leading edge of the containing box.

CDasherViewSquare::CTextString* CDasherViewSquare::DasherDrawText(myint iDasherMaxX, myint iDasherMidY, CDasherScreen::Label* pLabel, const ColorPalette::Color& Color)
{
	screenint x, y;
	Dasher2Screen(iDasherMaxX, iDasherMidY, x, y);

	// Old formulation based, where fontSize gave the smallest font size used:
	    //font size maxes out at ((iMaxY*3)/2)+iMaxY)/iMaxY = 3/2*smallest
	    // which is reached when iDasherMaxX == iMaxY/2, i.e. the crosshair (ORIGIN_X)
	    //const myint iSize = (std::min(iDasherMaxX * 3, (iMaxY * 3) / 2) + iMaxY) * m_pSettingsStore->GetLongParameter(LP_DASHER_FONTSIZE) / iMaxY;

	// New formulation, where fontSize gives the maximum font size, which is reached at the crosshair
	const float fSize = static_cast<float>(m_pSettingsStore->GetLongParameter(LP_DASHER_FONTSIZE));
    const float iSize = std::min(fSize,fSize * (0.6f * static_cast<float>(iDasherMaxX)/CDasherModel::ORIGIN_X + 0.4f)); // linear function passing through (0, fSize/2.5) and (CDasherModel::ORIGIN_X, fSize), capped at fSize

	return new CTextString(pLabel, x, y, static_cast<int>(iSize), Color);
}

void CDasherViewSquare::DoDelayedText(CTextString* pText, myint extrusionLevel, myint groupRecursionDepth)
{
	//note that it'd be better to compute old-style font sizes here, or even after shunting
	// across according to the aiMax array, but this needs Dasher co-ordinates, which were
	// more easily available at CTextString creation time. If it really doesn't look as good,
	// can put in extra calls to Screen2Dasher....
	// Coordinate Definitions <x,y>:
	// LR: <left, center>
	// RL: <right, center>
	// TB: <center, top>
	// BT: <center, lower>
	// x gives the coordinate of the side of the corresponding box
	// y gives the midpoint in y direction
	screenint x(pText->m_ix), y(pText->m_iy); 
	std::pair<screenint, screenint> textDims = Screen()->TextSize(pText->m_pLabel, pText->m_iSize);
	const bool extrudedText = m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE) == Options::CUBE;

	screenint textInset = m_pSettingsStore->GetLongParameter(LP_OUTLINE_WIDTH) + m_pSettingsStore->GetLongParameter(LP_TEXT_PADDING);
	
	switch (GetOrientation())
	{
	case Options::LeftToRight:
		{
	        const screenint iRight = x + textDims.first;
			if (x < Screen()->GetWidth() && iRight >= 0)
			{
				if(extrudedText)
				{
					Screen()->Draw3DLabel(pText->m_pLabel, x, y - textDims.second / 2, textInset, Options::LeftToRight, extrusionLevel, groupRecursionDepth, pText->m_iSize, pText->m_Color);
				}
				else
				{
					Screen()->DrawString(pText->m_pLabel, x + textInset, y - textDims.second / 2, pText->m_iSize, pText->m_Color);
				}
			}
			for (const auto& pChild : pText->m_children)
			{
				pChild->m_ix = std::max(pChild->m_ix, iRight);
				DoDelayedText(pChild, extrusionLevel + 1, 0);
			}
			pText->m_children.clear();
			break;
		}
	case Options::RightToLeft:
		{
            const screenint iLeft = x - textDims.first;
			if (iLeft < Screen()->GetWidth() && x >= 0)
			{
				if(extrudedText)
				{
					Screen()->Draw3DLabel(pText->m_pLabel, iLeft, y - textDims.second / 2, -textInset, Options::RightToLeft, extrusionLevel, groupRecursionDepth, pText->m_iSize, pText->m_Color);
				}
				else
				{
					Screen()->DrawString(pText->m_pLabel, iLeft - textInset, y - textDims.second / 2, pText->m_iSize, pText->m_Color);
				}
			}
			for (const auto& pChild : pText->m_children)
			{
				pChild->m_ix = std::min(pChild->m_ix, iLeft);
				DoDelayedText(pChild, extrusionLevel + 1, 0);
			}
			pText->m_children.clear();
			break;
		}
	case Options::TopToBottom:
		{
            const screenint iBottom = y + textDims.second;
			if (y < Screen()->GetHeight() && iBottom >= 0)
			{
				if(extrudedText)
				{
					Screen()->Draw3DLabel(pText->m_pLabel, x - textDims.first / 2, y, textInset, Options::TopToBottom, extrusionLevel, groupRecursionDepth, pText->m_iSize, pText->m_Color);
				}
				else
				{
					Screen()->DrawString(pText->m_pLabel, x - textDims.first / 2, y + textInset, pText->m_iSize, pText->m_Color);
				}
			}
			for (const auto& pChild : pText->m_children)
			{
				pChild->m_iy = std::max(pChild->m_iy, iBottom);
				DoDelayedText(pChild, extrusionLevel + 1, 0);
			}
			pText->m_children.clear();
			break;
		}
	case Options::BottomToTop:
		{
            const screenint iTop = y - textDims.second;
			if (iTop < Screen()->GetHeight() && y >= 0)
			{
				if(extrudedText)
				{
					Screen()->Draw3DLabel(pText->m_pLabel, x - textDims.first / 2, iTop + textDims.second, textInset, Options::BottomToTop, extrusionLevel, groupRecursionDepth, pText->m_iSize, pText->m_Color);
				}
				else
				{
					Screen()->DrawString(pText->m_pLabel, x - textDims.first / 2, iTop - textInset, pText->m_iSize, pText->m_Color);
				}
			}
			for (const auto& pChild : pText->m_children)
			{
				DoDelayedText(pChild, extrusionLevel + 1, 0);
			}
			pText->m_children.clear();
			break;
		}
	default:
		break;
	}
	delete pText;
}

CDasherViewSquare::CTextString::~CTextString()
{
	for (auto& child : m_children)
	{
		delete child;
	}
}

void CDasherViewSquare::TruncateTri(myint x, myint y1, myint y2, myint midy1, myint midy2, const ColorPalette::Color& fillColor, const ColorPalette::Color& outlineColor, int lineWidth)
{
	DASHER_ASSERT(y1 <= midy1 && midy1 <= midy2 && midy2 <= y2);
	
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();

	myint x1(x), x2(x); //(max)x-coords of the two lines
	myint tempx1(0), tempx2(0); //& min x-coords
	//intersect y1's diagonal with screen
	if (!ClipLineToVisible(x1, midy1, tempx1, y1))
	{
		//entirely offscreen....i.e. off top/bottom
		//DASHER_ASSERT (midy1 < visibleRegion.minY);//no, args undefined if returns false!
		midy1 = y1 = visibleRegion.minY;
		x1 = std::min(x1, visibleRegion.maxX);
		tempx1 = 0;
	}
	//intersect y2's diagonal with screen
	if (!ClipLineToVisible(tempx2, y2, x2, midy2))
	{
		//entirely offscreen again, i.e. off bottom/top
		midy2 = y2 = visibleRegion.maxY;
		x2 = std::min(x2, visibleRegion.maxX);
		tempx2 = 0;
	}
	if (x1 != x2)
	{
		//both will be clipped to be <= visibleRegion.maxX by above. If they are still
		// unequal, one must have been further clipped by passing off top/bottom
		// (i.e., the point of max x is off the top/off the bottom), in which case
		// the other line is entirely offscreen:
		DASHER_ASSERT(midy1 == midy2); //point/line of max x has been removed

		if (x1 < x2)
		{
			//(0,y1) - (x1,midy1) hit max-y edge of screen
			//(0,y2) - (x2,midy2) entirely offscreen
			DASHER_ASSERT(midy1 == visibleRegion.maxY && y2 == midy2);
			x2 = x1;
		}
		else
		{
			// (0,y2) - (x2, midy2) hit min-y edge of screen
			// (0,y1) - (x1,midy1) entirely offscreen
			DASHER_ASSERT(midy2 == visibleRegion.minY && y1 == midy1);
			x1 = x2;
		}
	}
	// midy1,x1 is now start point
	std::vector<point> pts(1);
	Dasher2Screen(x1, midy1, pts[0].x, pts[0].y);
	DasherLine2Screen(x1, midy1, tempx1, y1, pts);
	if (tempx1)
	{
		//did not reach y axis
		DASHER_ASSERT(y1 == visibleRegion.minY);
		pts.emplace_back();
		Dasher2Screen(0, visibleRegion.minY, pts.back().x, pts.back().y);
	}
	//that gets us to the min-y (y1) end of the line along the y-axis

	//add line along y-axis...
	pts.emplace_back();
	Dasher2Screen(0, y2, pts.back().x, pts.back().y);

	if (tempx2)
	{
		//y2's diagonal did not reach y-axis
		DASHER_ASSERT(y2 == visibleRegion.maxY);
		pts.emplace_back();
		Dasher2Screen(tempx2, visibleRegion.maxY, pts.back().x, pts.back().y);
	}
	//and the diagonal part...
	DasherLine2Screen(tempx2, y2, x2, midy2, pts);

	if (midy1 != midy2)
	{
		//is the max-x extent a line, after cropping - i.e. handles both
		// normal triangle (orig midy1==orig midy2) being cropped to screen edge,
		// and trunc tri (orig midy1 < orig midy2, possibly cropped) cases
		DASHER_ASSERT(x1 == x2);
		pts.emplace_back();
		Dasher2Screen(x1, midy1, pts.back().x, pts.back().y);
	}
	else
		DASHER_ASSERT(pts.back().x == pts[0].x && pts.back().y == pts[0].y);

	auto p_array = new point[pts.size()];
	for (unsigned int i = 0; i < pts.size(); i++) p_array[i] = pts[i];
	Screen()->Polygon(p_array, static_cast<int>(pts.size()), fillColor, outlineColor, lineWidth);
	delete[] p_array;
}

#define sq(X) ((X)*(X))

void CDasherViewSquare::Circle(myint Range, myint y1, myint y2, const ColorPalette::Color& fillColor, const ColorPalette::Color& outlineColor, int lineWidth)
{
	std::vector<point> pts;
	myint cy((y1 + y2) / 2), r(Range / 2), x1, x2;
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();

	point p;
	//run along bottom edge...
	if (y1 < visibleRegion.minY)
	{
		Dasher2Screen(0, visibleRegion.minY, p.x, p.y);
		pts.push_back(p);
		//intersect with bottom edge
		x1 = std::min(visibleRegion.maxX, static_cast<myint>(sqrt(static_cast<double>(r * r - sq(cy - visibleRegion.minY)))));
		y1 = visibleRegion.minY;
	}
	else
	{
		x1 = 0;
	}
	Dasher2Screen(x1, y1, p.x, p.y);
	pts.push_back(p);

	//and along top...
	if (y2 > visibleRegion.maxY)
	{
		//intersect...
		x2 = std::min(visibleRegion.maxX, static_cast<myint>(sqrt(static_cast<double>(r * r - sq(visibleRegion.maxY - cy)))));
		Dasher2Screen(x2, y2 = visibleRegion.maxY, p.x, p.y);
		//that's target point for end of curved section.
		if (x2 == visibleRegion.maxX && x1 == visibleRegion.maxX)
		{
			//circle entirely covers screen
			DASHER_ASSERT(y1 == visibleRegion.minY);
			DasherDrawRectangle(visibleRegion.maxX, visibleRegion.minY, 0, visibleRegion.maxY, fillColor, outlineColor, lineWidth);
			return;
		}
		//will also need final point at top-right (0,y2 in dasher coords)....
	}
	else
	{
		Dasher2Screen(x2 = 0, y2, p.x, p.y);
	}
	CircleTo(cy, r, y1, x1, y2, x2, p, pts, 2.0);
	if (visibleRegion.maxY == y2)
	{
		Dasher2Screen(0, visibleRegion.maxX, p.x, p.y);
		pts.push_back(p);
	}
	auto p_array = new point[pts.size()];
	for (unsigned int i = 0; i < pts.size(); i++) p_array[i] = pts[i];
	Screen()->Polygon(p_array, static_cast<int>(pts.size()), fillColor, outlineColor, lineWidth);
	delete[] p_array;
}

void CDasherViewSquare::CircleTo(myint cy, myint r, myint y1, myint x1, myint y3, myint x3, point dest, std::vector<point>& pts, double dXMul)
{
	myint y2((y1 + y3) / 2);
	myint x2(static_cast<myint>(sqrt((sq(r) - sq(cy - y2)) * dXMul)));
	point mid; //where midpoint of circle/arc should be
	Dasher2Screen(x2, y2, mid.x, mid.y); //(except "midpoint" measured along y axis)
	int lmx = (pts.back().x + dest.x) / 2, lmy = (pts.back().y + dest.y) / 2; //midpoint of straight line
	if (abs(dest.y - pts.back().y) < 2 || abs(mid.x - lmx) + abs(mid.y - lmy) < 2)
	{
		//okay, use straight line
		pts.push_back(dest);
	}
	else
	{
		CircleTo(cy, r, y1, x1, y2, x2, mid, pts, dXMul);
		CircleTo(cy, r, y2, x2, y3, x3, dest, pts, dXMul);
	}
}
#undef sq

void CDasherViewSquare::DasherSpaceArc(myint cy, myint r, myint x1, myint y1, myint x2, myint y2, const ColorPalette::Color& color, int iLineWidth)
{
	point p;
	//start point
	Dasher2Screen(x1, y1, p.x, p.y);
	std::vector<point> pts;
	pts.push_back(p);
	//if circle goes behind crosshair and we want the point of max-x, force division into two sections with that point as boundary
	if (r > CDasherModel::ORIGIN_X && ((y1 < cy) ^ (y2 < cy)))
	{
		Dasher2Screen(r, cy, p.x, p.y);
		CircleTo(cy, r, y1, x1, cy, r, p, pts, 1.0);
		x1 = r;
		y1 = cy;
	}
	Dasher2Screen(x2, y2, p.x, p.y);
	CircleTo(cy, r, y1, x1, y2, x2, p, pts, 1.0);
	auto p_array = new point[pts.size()];
	for (unsigned int i = 0; i < pts.size(); i++) p_array[i] = pts[i];
	Screen()->Polyline(p_array, static_cast<int>(pts.size()), iLineWidth, color);
}

void CDasherViewSquare::Quadric(myint Range, myint lowY, myint highY, const ColorPalette::Color& fillColor, const ColorPalette::Color& outlineColor, int lineWidth)
{
	static const double RR2 = 1.0 / sqrt(2.0);
	const int midY = static_cast<int>((lowY + highY) / 2);
#define NUM_STEPS 40
	point p_array[2 * NUM_STEPS + 2];
	
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();
	{
		myint x1(0), y1(highY), x2(static_cast<myint>(Range * RR2)), y2(static_cast<myint>(highY * RR2 + midY * (1.0 - RR2))), x3(Range), y3(midY);
		for (int i = 0; i <= NUM_STEPS; i++)
		{
			double f = i / static_cast<double>(NUM_STEPS), of = 1.0 - f;
			Dasher2Screen(std::min(visibleRegion.maxX, static_cast<myint>(of * of * x1 + 2.0 * of * f * x2 + f * f * x3)), std::max(visibleRegion.minY, std::min(visibleRegion.maxY, static_cast<myint>(of * of * y1 + 2.0 * of * f * y2 + f * f * y3))), p_array[i].x, p_array[i].y);
		}
	}
	{
		myint x1(Range), y1(midY), x2(static_cast<myint>(Range * RR2)), y2(static_cast<myint>(lowY * RR2 + midY * (1.0 - RR2))), x3(0), y3(lowY);
		for (int i = 0; i <= NUM_STEPS; i++)
		{
			double f = i / static_cast<double>(NUM_STEPS), of = 1.0 - f;
			Dasher2Screen(std::min(visibleRegion.maxX, static_cast<myint>(of * of * x1 + 2.0 * of * f * x2 + f * f * x3)), std::max(visibleRegion.minY, std::min(visibleRegion.maxY, static_cast<myint>(of * of * y1 + 2.0 * of * f * y2 + f * f * y3))), p_array[i + NUM_STEPS + 1].x, p_array[i + NUM_STEPS + 1].y);
		}
	}

	Screen()->Polygon(p_array, 2 * NUM_STEPS + 2, fillColor, outlineColor, lineWidth);
#undef NUM_STEPS
}

bool CDasherViewSquare::IsSpaceAroundNode(myint y1, myint y2)
{
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();
	const myint maxX(y2 - y1);
	if ((maxX < visibleRegion.maxX) || (y1 > visibleRegion.minY) || (y2 < visibleRegion.maxY))
		return true; //space around sq => space around anything smaller!

	//in theory, even if the crosshair is off-screen (!), anything spanning y1-y2 should cover it...
	DASHER_ASSERT(CoversCrosshair(y2 - y1, y1, y2));

	switch (m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE))
	{
	case Options::DISJOINT_RECTANGLE:
	case Options::OVERLAPPING_RECTANGLE:
	case Options::CUBE:
		return false;
	case Options::TRIANGLE:
		{
			const myint iMidY((y1 + y2) / 2);
			return (iMidY < visibleRegion.maxY && (y2 - visibleRegion.maxY) * maxX < visibleRegion.maxX * (y2 - iMidY))
				|| (iMidY > visibleRegion.minY && (visibleRegion.minY - y1) * maxX < visibleRegion.maxX * (iMidY - y1));
		}
	case Options::TRUNCATED_TRIANGLE:
		{
			const myint y113((y1 + y1 + y2) / 3), y123((y1 + y2 + y2) / 3);
			return (y123 < visibleRegion.maxY && (y2 - visibleRegion.maxY) * maxX < visibleRegion.maxX * (y2 - y123))
				|| (y113 > visibleRegion.minY && (visibleRegion.minY - y1) * maxX < visibleRegion.maxX * (y123 - y1));
		}
	case Options::QUADRIC: //quadric.
	//erm. seems hard. fall-through to circle, as it isn't far out -
	// unfortunately it's not a conservative approximation, the circle
	// covers the quadric not the other way around, so we'll say the
	// circle covers the screen when the quadric doesn't :-(. However
	// atm circles seem better generally so fixing quadrics is a low priority!
	case Options::CIRCLE:
		{
			//circle - or rather ellipse, x diameter is twice y diam, hence the *2 to normalize
			const myint iMidY((y1 + y2) / 2); //centerX=0, radius = maxX
			const myint maxYDiff(std::max(visibleRegion.maxY - iMidY, iMidY - visibleRegion.minY) * 2);
			return maxYDiff * maxYDiff + visibleRegion.maxX * visibleRegion.maxX > maxX * maxX;
		}
	}
	/* NOTREACHED */
	return false;
}

void CDasherViewSquare::DisjointRender(CDasherNode* pRender, myint y1, myint y2,
                                       CTextString* pPrevText, CExpansionPolicy& policy, double dMaxCost,
                                       CDasherNode*& pOutput)
{
	DASHER_ASSERT_VALIDPTR_RW(pRender);
	
	// Set the NF_SUPER flag if this node entirely frames the visual
	// area.

	const DasherCoordScreenRegion visibleRegion = VisibleRegion();
	pRender->SetFlag(CDasherNode::NF_SUPER, (y2 - y1 >= visibleRegion.maxX) && (y1 <= visibleRegion.minY) && (y2 >= visibleRegion.maxY));

	if (pRender->getLabel())
	{
		myint ny1 = std::min(visibleRegion.maxY, std::max(visibleRegion.minY, y1)),
		      ny2 = std::min(visibleRegion.maxY, std::max(visibleRegion.minY, y2));
		CTextString* pText = DasherDrawText(y2 - y1, (ny1 + ny2) / 2, pRender->getLabel(), pRender->getLabelColor(m_pColorPalette));
		((pPrevText) ? pPrevText->m_children : m_DelayedTexts).push_back(pText); // add text at appropriate queue

		if (pRender->bShove()) pPrevText = pText;
	}

	const myint Range(y2 - y1);

	//Does node cover crosshair?
	if (pOutput == pRender->Parent() && Range > CDasherModel::ORIGIN_X && y1 < CDasherModel::ORIGIN_Y && y2 > CDasherModel::ORIGIN_Y)
	{
		pOutput = pRender;
		if (pRender->ChildCount() == 0)
		{
			//covers crosshair! forcibly populate, now!
			policy.ExpandNode(pRender);
		}
	}
	if (pRender->ChildCount() == 0)
	{
		//allow empty node to be expanded, it's big enough.
		policy.pushNode(pRender, static_cast<int>(y1), static_cast<int>(y2), true, dMaxCost);
		//and render whole node in one go
		DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), 0, std::min(y2, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);
		//fall through to draw outline
	}
	else
	{
		//Node has children. It can therefore be collapsed...however,
		// we don't allow a node covering the crosshair to be collapsed
		// (at best this'll mean there's nowhere useful to go forwards;
		// at worst, all kinds of crashes trying to do text output!)

		//No reason why we can't collapse a game mode node that's too small/offscreen
		// - we've got its coordinates, and can recreate its children and set their
		// NF_GAME flags appropriately when it becomes renderable again...
		if (pRender != pOutput)
			dMaxCost = policy.pushNode(pRender, static_cast<int>(y1), static_cast<int>(y2), false, dMaxCost);

		// Render children

		int id = -1;

		if (CDasherNode* pChild = pRender->onlyChildRendered)
		{
			//if child still covers screen, render _just_ it and return
			myint newy1 = y1 + (Range * pChild->Lbnd()) / CDasherModel::NORMALIZATION;
			myint newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;
			if (newy1 < visibleRegion.minY && newy2 > visibleRegion.maxY)
			{
				//still covers entire screen. Parent should too...
				DASHER_ASSERT(dMaxCost == std::numeric_limits<double>::infinity());

				if (newy2 - newy1 < visibleRegion.maxX) //fill in to it's left...
					DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), newy2 - newy1, std::min(y2, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);
				DisjointRender(pChild, newy1, newy2, pPrevText,
				               policy, dMaxCost, pOutput);
				//leave pRender->onlyChildRendered set, so remaining children are skipped
			}
			else
				pRender->onlyChildRendered = nullptr;
		}

		if (!pRender->onlyChildRendered)
		{
			//render all children...
			myint lasty = y1;
			for (auto i = pRender->GetChildren().begin();
			     i != pRender->GetChildren().end(); ++i)
			{
				id++;
				CDasherNode* pChild = *i;

				myint newy1 = y1 + (Range * pChild->Lbnd()) / CDasherModel::NORMALIZATION;
				myint newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;

				if (pChild->GetFlag(CDasherNode::NF_GAME))
				{
					OnGameNodeDraw.Broadcast(pChild, newy1, newy2);
				}
				//switch to "render just one child" mode if all others are off the screen,
				//and if this _won't_ cause us to avoid rendering a game node...
				if (newy1 < visibleRegion.minY && newy2 > visibleRegion.maxY && (!pRender->GetFlag(CDasherNode::NF_GAME) || pChild->GetFlag(CDasherNode::NF_GAME)))
				{
					DASHER_ASSERT(dMaxCost == std::numeric_limits<double>::infinity());
					pRender->onlyChildRendered = pChild;
					if (newy2 - newy1 < visibleRegion.maxX)
						DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), newy2 - newy1, std::min(y2, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);
					DisjointRender(pChild, newy1, newy2, pPrevText, policy, dMaxCost, pOutput);
					//ensure we don't blank over this child in "finishing off" the parent (!)
					lasty = newy2;
					//all remaining children are offscreen. quickly delete, avoid recomputing ranges...
					while ((++i) != pRender->GetChildren().end())
						if (!(*i)->GetFlag(CDasherNode::NF_SEEN)) (*i)->DeleteChildren();
					break;
				}
				if (newy2 - newy1 >= m_pSettingsStore->GetLongParameter(LP_MIN_NODE_SIZE) //simple test if big enough
					&& newy1 <= visibleRegion.maxY && newy2 >= visibleRegion.minY) //at least partly on screen
				{
					//child should be rendered!
					//fill in to its left
					DasherDrawRectangle(std::min(y2 - y1, visibleRegion.maxX), std::max(newy1, visibleRegion.minY), std::min(newy2 - newy1, visibleRegion.maxX), std::min(newy2, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);

					if (std::max(lasty, visibleRegion.minY) < newy1) //fill in interval above child up to the last drawn child
						DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(lasty, visibleRegion.minY), 0, std::min(newy1, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);
					lasty = newy2;
					DisjointRender(pChild, newy1, newy2, pPrevText, policy, dMaxCost, pOutput);
				}
				else
				{
					// We get here if the node is too small to render or is off-screen.
					// So, collapse it immediately.
					if (!pChild->GetFlag(CDasherNode::NF_SEEN))
						pChild->DeleteChildren();
				}
			}
			//all children rendered.
			if (lasty < std::min(y2, visibleRegion.maxY))
			{
				// Finish off the drawing process, filling in any part of the parent below the last-rendered child
				DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(lasty, visibleRegion.minY), 0, std::min(y2, visibleRegion.maxY), pRender->getNodeColor(m_pColorPalette), pRender->getOutlineColor(m_pColorPalette), 0);
			}
		}
		//end rendering children, fall through to outline
	}
	// Lastly, draw the outline
	if (m_pSettingsStore->GetLongParameter(LP_OUTLINE_WIDTH) && !pRender->getOutlineColor(m_pColorPalette).isFullyTransparent())
	{
		DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), 0, std::min(y2, visibleRegion.maxY), ColorPalette::noColor, pRender->getOutlineColor(m_pColorPalette), labs(m_pSettingsStore->GetLongParameter(LP_OUTLINE_WIDTH)));
	}
}

void CDasherViewSquare::DasherDrawCube(myint iDasherMaxX, myint iDasherMinY, myint iDasherMinX, myint iDasherMaxY, CubeDepthLevel nodeDepth, CubeDepthLevel parentDepth, const ColorPalette::Color& Color, const ColorPalette::Color& outlineColor, int iThickness, ScreenRegion* parentScreenBounds)
{
	screenint iScreenMaxX, iScreenMinY, iScreenMinX, iScreenMaxY;
	Dasher2Screen(iDasherMaxX, iDasherMinY, iScreenMaxX, iScreenMinY);
	Dasher2Screen(iDasherMinX, iDasherMaxY, iScreenMinX, iScreenMaxY);

	if(iScreenMaxX < iScreenMinX) std::swap(iScreenMaxX,iScreenMinX);
	if(iScreenMaxY < iScreenMinY) std::swap(iScreenMaxY,iScreenMinY);


	if(parentScreenBounds){
	    //Clip to parentBounds
	    iScreenMaxX = std::min(parentScreenBounds->maxX, iScreenMaxX);
	    iScreenMaxY = std::min(parentScreenBounds->maxY, iScreenMaxY);
	    iScreenMinX = std::max(parentScreenBounds->minX, iScreenMinX);
	    iScreenMinY = std::max(parentScreenBounds->minY, iScreenMinY);

	    parentScreenBounds->minX = iScreenMinX;
	    parentScreenBounds->minY = iScreenMinY;
	    parentScreenBounds->maxX = iScreenMaxX;
	    parentScreenBounds->maxY = iScreenMaxY;
	}

	const float sizeX = static_cast<float>(iScreenMaxX - iScreenMinX);
	const float sizeY = static_cast<float>(iScreenMaxY - iScreenMinY);

	Screen()->DrawCube(static_cast<float>(iScreenMinX) + (sizeX / 2.0f), static_cast<float>(iScreenMinY) + (sizeY / 2.0f), sizeX, sizeY, nodeDepth, parentDepth, Color, outlineColor, iThickness);
}

bool CDasherViewSquare::CoversCrosshair(myint Range, myint y1, myint y2)
{
	if (Range > CDasherModel::ORIGIN_X && y1 < CDasherModel::ORIGIN_Y && y2 > CDasherModel::ORIGIN_Y)
	{
		switch (m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE))
		{
		case Options::DISJOINT_RECTANGLE:
		case Options::OVERLAPPING_RECTANGLE:
		case Options::CUBE:
			return true;
		case Options::TRIANGLE:
			{
				//Triangles
				myint iMidY((y1 + y2) / 2);
				return (iMidY > CDasherModel::ORIGIN_Y)
					       ? ((CDasherModel::ORIGIN_Y - y1) * Range) > (iMidY - y1) * CDasherModel::ORIGIN_X
					       : ((y2 - CDasherModel::ORIGIN_Y) * Range) > (y2 - iMidY) * CDasherModel::ORIGIN_X;
			}
		case Options::TRUNCATED_TRIANGLE:
			{
				//Truncated tris
				myint midy1((y1 + y1 + y2) / 3), midy2((y1 + y2 + y2) / 3);
				if (midy1 > CDasherModel::ORIGIN_Y) //(0,y1) - (Range,midy1)
					return (CDasherModel::ORIGIN_Y - y1) * Range > (midy1 - y1) * CDasherModel::ORIGIN_X;
				if (midy2 > CDasherModel::ORIGIN_Y) // (Range,midy1) - (Range,midy2)
					return true;
				return (y2 - CDasherModel::ORIGIN_Y) * Range > (y2 - midy2) * CDasherModel::ORIGIN_X;
				break;
			}
		case Options::QUADRIC: //quadrics. We'll approximate with circles, as they're easier...
		// however, note that the circle is bigger, so this'll output things
		// too soon/aggressively :-(.
		// (hence, fallthrough to:)
		case Options::CIRCLE:
			{
				//circles - actually ellipses, as x diameter is twice y diameter, hence the *4
				const myint y_dist(CDasherModel::ORIGIN_Y - (y1 + y2) / 2);
				return CDasherModel::ORIGIN_X * CDasherModel::ORIGIN_X + y_dist * y_dist * 4 < Range * Range;
			}
		}
	}
	return false;
}

ColorPalette::Color CDasherViewSquare::SimulateTransparency(CDasherNode* pCurrentNode)
{
	if(pCurrentNode == nullptr) return GetNamedColor(NamedColor::background);

    const ColorPalette::Color nodeColor = pCurrentNode->getNodeColor(m_pColorPalette);

	if(nodeColor.isFullyOpaque()) return nodeColor; // no transparency
	
	//Interpolate between this and the parent
	const ColorPalette::Color parentColor = SimulateTransparency(pCurrentNode->Parent());
	ColorPalette::Color interpolatedColor = ColorPalette::Color::lerp(nodeColor, parentColor, static_cast<float>(nodeColor.Alpha) / 255.0f);
	interpolatedColor.Alpha = 255;
	return interpolatedColor;
}

void CDasherViewSquare::NewRender(CDasherNode* pCurrentNode, myint y1, myint y2,
                                  CTextString* pPrevText, CExpansionPolicy& policy, double dMaxCost,
                                  CDasherNode*& pCurrentTopCenterNode, CubeDepthLevel nodeDepth, CubeDepthLevel parentDepth, ScreenRegion parentScreenBounds)
{
	DASHER_ASSERT_VALIDPTR_RW(pRender);
	
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();

	// Set the NF_SUPER flag if this node entirely frames the visual area.
	// This causes the model to adjust its root. Do not change the root node, if it would be fully transparent
	// as this leads to part of the screen not being rendered correctly
	pCurrentNode->SetFlag(CDasherNode::NF_SUPER, (!IsSpaceAroundNode(y1, y2) && !pCurrentNode->getNodeColor(m_pColorPalette).isFullyTransparent()));

	if(!pCurrentNode->getLabel())
	{
		nodeDepth.extrusionLevel = parentDepth.extrusionLevel; //Use same level as node before
		nodeDepth.groupRecursionDepth++;
	} else
	{
		nodeDepth.groupRecursionDepth = 0;
	}

	if (pCurrentNode->getLabel())
	{
		myint ny1 = std::min(visibleRegion.maxY, std::max(visibleRegion.minY, y1)),
		      ny2 = std::min(visibleRegion.maxY, std::max(visibleRegion.minY, y2));
		CTextString* pText = DasherDrawText(y2 - y1, (ny1 + ny2) / 2, pCurrentNode->getLabel(), pCurrentNode->getLabelColor(m_pColorPalette));

		// add text at appropriate queue
		if(pPrevText){
			pPrevText->m_children.push_back(pText);
		} else {
			if(m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE) == Options::CUBE)
			{
				m_Delayed3DTexts.push_back({pText, nodeDepth.extrusionLevel, nodeDepth.groupRecursionDepth});
			}
			else
			{
				m_DelayedTexts.push_back(pText);
			}
		}

		if (pCurrentNode->bShove()) pPrevText = pText;
	}

	const myint Range(y2 - y1);
	// Draw node...we can both fill & outline in one go, since
	// we're drawing the whole node at once (unlike disjoint-rects),
	// as any part of the outline which is obscured by a child node,
	// will have the outline of the child node painted over it,
	// and all outlines are the same color.

	//"invisible" nodes are given same color as parent, so we neither fill
	// nor outline them. TODO this isn't quite right, as nodes that are
	// _supposed_ to be the same color as their parent, will have no outlines...
	// (thankfully having 2 "phases" means this doesn't happen in standard
	// color schemes)
	if (!pCurrentNode->getNodeColor(m_pColorPalette).isFullyTransparent())
	{
		//outline width 0 = fill only; >0 = fill + outline; <0 = outline only
		const int line_width = labs(m_pSettingsStore->GetLongParameter(LP_OUTLINE_WIDTH));
		const ColorPalette::Color& fill_color = line_width < 0 ? ColorPalette::noColor : (m_pSettingsStore->GetBoolParameter(BP_SIMULATE_TRANSPARENCY) ? SimulateTransparency(pCurrentNode) : pCurrentNode->getNodeColor(m_pColorPalette));
		const ColorPalette::Color& outline_color = line_width == 0 ? ColorPalette::noColor : pCurrentNode->getOutlineColor(m_pColorPalette);

	    switch (m_pSettingsStore->GetLongParameter(LP_SHAPE_TYPE))
		{
		case Options::OVERLAPPING_RECTANGLE:
	        DasherDrawRectangle(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), 0, std::min(y2, visibleRegion.maxY), fill_color, outline_color, line_width);
			break;
		case Options::TRIANGLE:
			TruncateTri(Range, y1, y2, (y1 + y2) / 2, (y1 + y2) / 2, fill_color, outline_color, line_width);
			break;
		case Options::TRUNCATED_TRIANGLE:
			TruncateTri(Range, y1, y2, (y1 + y1 + y2) / 3, (y1 + y2 + y2) / 3, fill_color, outline_color, line_width);
			break;
		case Options::QUADRIC:
			Quadric(Range, y1, y2, fill_color, outline_color, line_width);
			break;
		case Options::CIRCLE:
			Circle(Range, y1, y2, fill_color, outline_color, line_width);
			break;
		case Options::CUBE:
			DasherDrawCube(std::min(Range, visibleRegion.maxX), std::max(y1, visibleRegion.minY), 0, std::min(y2, visibleRegion.maxY), nodeDepth, parentDepth, fill_color, outline_color, line_width, &parentScreenBounds);
			break;
		default: break;
        }
	}

	// Are we a decendent of the current crosshair covering node?
	// + Do we cover the crosshair
	// Covering means (below crosshair)
	if (pCurrentTopCenterNode == pCurrentNode->Parent() && CoversCrosshair(Range, y1, y2))
	{
		pCurrentTopCenterNode = pCurrentNode;
		m_CrosshairCubeLevel = nodeDepth.extrusionLevel; //Current extrusionLevel
	}

	if (pCurrentNode->ChildCount() == 0)
	{
		if (pCurrentTopCenterNode == pCurrentNode)
		{
			//covers crosshair! forcibly populate, now!
			policy.ExpandNode(pCurrentNode);
		}
		else
		{
			//allow empty node to be expanded, it's big enough.
			policy.pushNode(pCurrentNode, static_cast<int>(y1), static_cast<int>(y2), true, dMaxCost);
			return; //no children atm => nothing more to do
		}
	}
	else
	{
		//Node has children. It can therefore be collapsed...however,
		// we don't allow a node covering the crosshair to be collapsed
		// (at best this'll mean there's nowhere useful to go forwards;
		// at worst, all kinds of crashes trying to do text output!)

		//No reason why we can't collapse a game mode node that's too small/offscreen
		// - we've got its coordinates, and can recreate its children and set their
		// NF_GAME flags appropriately when it becomes renderable again...
		if (pCurrentNode != pCurrentTopCenterNode)
			dMaxCost = policy.pushNode(pCurrentNode, static_cast<int>(y1), static_cast<int>(y2), false, dMaxCost);
	}
	//Node has children - either it already did, or else it covers the crosshair,
	// and we've just made them...so render them.

    const CubeDepthLevel nextLevel = {nodeDepth.extrusionLevel + 1, nodeDepth.groupRecursionDepth};
	//first check if there's only one child we need to render
	if (CDasherNode* pChild = pCurrentNode->onlyChildRendered)
	{
		//if child still covers screen, render _just_ it and return
		myint newy1 = y1 + (Range * pChild->Lbnd()) / CDasherModel::NORMALIZATION;
		myint newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;
		if (
			(newy1 < visibleRegion.minY && newy2 > visibleRegion.maxY))
		{
			//covers entire y-axis!
			//render just that child; nothing more to do for this node => tail call to beginning
			NewRender(pChild, newy1, newy2, pPrevText, policy, dMaxCost, pCurrentTopCenterNode, nextLevel, nodeDepth, parentScreenBounds);
			return;
		}
		pCurrentNode->onlyChildRendered = nullptr;
	}

	//ok, need to render all children...
	myint newy1 = y1;
	auto I = pCurrentNode->GetChildren().begin(), E = pCurrentNode->GetChildren().end();
	while (I != E)
	{
		CDasherNode* pChild(*I);

		myint newy2 = y1 + (Range * pChild->Hbnd()) / CDasherModel::NORMALIZATION;
		if (pChild->GetFlag(CDasherNode::NF_GAME))
		{
			OnGameNodeDraw.Broadcast(pChild, newy1, newy2);
		}
		if (newy1 <= visibleRegion.maxY && newy2 >= visibleRegion.minY)
		{
			//onscreen
			if (newy2 - newy1 > m_pSettingsStore->GetLongParameter(LP_MIN_NODE_SIZE))
			{
				//definitely big enough to render.
				NewRender(pChild, newy1, newy2, pPrevText, policy, dMaxCost, pCurrentTopCenterNode, nextLevel, nodeDepth, parentScreenBounds);
			}
			else if (!pChild->GetFlag(CDasherNode::NF_SEEN)) pChild->DeleteChildren();
			if (newy2 > visibleRegion.maxY && !pCurrentNode->GetFlag(CDasherNode::NF_GAME))
			{
				//remaining children offscreen and no game-mode child we might skip
				// (among the remainder, or any previous off the top of the screen)
				if (newy1 < visibleRegion.minY) pCurrentNode->onlyChildRendered = pChild; //previous children also offscreen!
				break; //skip remaining children
			}
		}
		++I;
		newy1 = newy2;
	}
	if (I != E)
	{
		//broke out of loop. Possibly more to delete...
		while (++I != E) if (!(*I)->GetFlag(CDasherNode::NF_SEEN)) (*I)->DeleteChildren();
	}
	//all children rendered.
}

/// Convert screen co-ordinates to dasher co-ordinates. This doesn't
/// include the nonlinear mapping for eyetracking mode etc - it is
/// just the inverse of the mapping used to calculate the screen
/// positions of boxes etc.

void CDasherViewSquare::Screen2Dasher(screenint iInputX, screenint iInputY, myint& iDasherX, myint& iDasherY)
{
	// Things we're likely to need:

	screenint iScreenWidth = Screen()->GetWidth();
	screenint iScreenHeight = Screen()->GetHeight();

	switch (GetOrientation())
	{
	case Options::LeftToRight:
		iDasherX = (iScreenWidth - iInputX) * SCALE_FACTOR / iScaleFactorX;
		iDasherY = CDasherModel::MAX_Y / 2 + (iInputY - iScreenHeight / 2) * SCALE_FACTOR / iScaleFactorY;
		break;
	case Options::RightToLeft:
		iDasherX = iInputX * SCALE_FACTOR / iScaleFactorX;
		iDasherY = CDasherModel::MAX_Y / 2 + (iInputY - iScreenHeight / 2) * SCALE_FACTOR / iScaleFactorY;
		break;
	case Options::TopToBottom:
		iDasherX = (iScreenHeight - iInputY) * SCALE_FACTOR / iScaleFactorX;
		iDasherY = CDasherModel::MAX_Y / 2 + (iInputX - iScreenWidth / 2) * SCALE_FACTOR / iScaleFactorY;
		break;
	case Options::BottomToTop:
		iDasherX = iInputY * SCALE_FACTOR / iScaleFactorX;
		iDasherY = CDasherModel::MAX_Y / 2 + (iInputX - iScreenWidth / 2) * SCALE_FACTOR / iScaleFactorY;
		break;
	default:
		break;
	}

	iDasherX = ixmap(iDasherX);
	iDasherY = iymap(iDasherY);
}

void CDasherViewSquare::SetScaleFactor(void)
{
	//Parameters for X non-linearity.
	// Set some defaults here, in case we change(d) them later...
	m_iXlogThres = CDasherModel::MAX_Y / 2; //threshold: DasherX's less than this are linear; those greater are logarithmic

	//set log scaling coefficient (unused if LP_NONLINEAR_X==0)
	// note previous value = 1/0.2, i.e. a value of LP_NONLINEAR_X =~= 4.8
	m_dXlogCoeff = exp(m_pSettingsStore->GetLongParameter(LP_NONLINEAR_X) / 3.0);

	const bool bHoriz(GetOrientation() == Options::LeftToRight || GetOrientation() == Options::RightToLeft);
	const screenint iScreenWidth(Screen()->GetWidth()), iScreenHeight(Screen()->GetHeight());
	const double dPixelsX(bHoriz ? iScreenWidth : iScreenHeight), dPixelsY(bHoriz ? iScreenHeight : iScreenWidth);

	//Defaults/starting values, will be modified later according to scheme in use...
	iMarginWidth = m_pSettingsStore->GetLongParameter(LP_MARGIN_WIDTH);
	double dScaleFactorY(dPixelsY / CDasherModel::MAX_Y);
	double dScaleFactorX(dPixelsX / static_cast<double>(CDasherModel::MAX_Y + iMarginWidth));

	switch (m_pSettingsStore->GetLongParameter(LP_GEOMETRY))
	{
	case Options::ScreenGeometry::old_style:
		{
			//old style
			if (dScaleFactorX < dScaleFactorY)
			{
				//fewer (pixels per dasher coord) in X direction - i.e., X is more compressed.
				//So, use X scale for Y too...except first, we'll _try_ to reduce the difference
				// by changing the relative scaling of X and Y (by at most 20%):
				double dMul = std::max(0.8, dScaleFactorX / dScaleFactorY);
				dScaleFactorY = std::max(dScaleFactorX / dMul, dScaleFactorY / 4.0);
				dScaleFactorX *= 0.9;
				iMarginWidth = static_cast<myint>((CDasherModel::MAX_Y / 20.0 + iMarginWidth * 0.95) / 0.9);
			}
			else
			{
				//X has more room; use Y scale for both -> will get lots history
				// however, "compensate" by relaxing the default "relative scaling" of X
				// (normally only 90% of Y) towards 1...
				double dXmpc = std::min(1.0, 0.9 * dScaleFactorX / dScaleFactorY);
				dScaleFactorX = std::max(dScaleFactorY, dScaleFactorX / 4.0) * dXmpc;
				iMarginWidth = static_cast<myint>((iMarginWidth + dPixelsX / dScaleFactorX - CDasherModel::MAX_Y) / 2);
			}
			break;
		}
	//all new styles fix the y axis the way we want it (i.e. leave as above),
	// and just do different things with x...
	case Options::ScreenGeometry::square_no_xhair:
		//square with x-hair possibly offscreen
		dScaleFactorX = dScaleFactorY;
		break;
	case Options::ScreenGeometry::squish:
	case Options::ScreenGeometry::squish_and_log:
		//2 or 3 => squish x (so xhair always visible)
		const double dDesiredXPerPixel((CDasherModel::MAX_Y + iMarginWidth) / dPixelsX), dMinXPerPixel((CDasherModel::ORIGIN_X + iMarginWidth) / dPixelsX);
		const double dAspect(1.0 / dScaleFactorY / dDesiredXPerPixel);
		double dDasherXPerPixel((dAspect < 1.0)
			                        ? (dMinXPerPixel + pow(dAspect, 3.0) * (dDesiredXPerPixel - dMinXPerPixel)) //tall+thin
			                        : (1.0 / dScaleFactorY)); //square or wide+low
		iMarginWidth = static_cast<myint>(iMarginWidth / 0.9); //this comes from the old scaling by m_dXmpc=0.9. Drop in new scheme?
		if (m_pSettingsStore->GetLongParameter(LP_GEOMETRY) == Options::ScreenGeometry::squish_and_log)
		{
			//make whole screen logarithmic (but keep xhair in same place)
			myint crosshair(xmap(2048)); //should be 2048...
			m_iXlogThres = 0;
			dDasherXPerPixel *= xmap(2048) / static_cast<double>(crosshair);
		}
		dScaleFactorX = 0.9 / dDasherXPerPixel;

		break;
	}
	iScaleFactorX = static_cast<myint>(dScaleFactorX * SCALE_FACTOR);
	iScaleFactorY = static_cast<myint>(dScaleFactorY * SCALE_FACTOR);

	//notify listeners that coordinates have changed...
	OnGeometryChanged.Broadcast();
}


inline myint CDasherViewSquare::CustomIDivScaleFactor(myint iNumerator)
{
	// Integer division rounding away from zero
	myint res;

	DASHER_ASSERT(SCALE_FACTOR != 0);

	auto [quot, rem] = std::lldiv(iNumerator, SCALE_FACTOR);

	if (rem < 0)
		res = quot - 1;
	else if (rem > 0)
		res = quot + 1;
	else
		res = quot;

	return res;

	// return (iNumerator + iDenominator - 1) / iDenominator;
}

void CDasherViewSquare::Dasher2Screen(myint iDasherX, myint iDasherY, screenint& iScreenX, screenint& iScreenY)
{
	// Apply the nonlinearities

	iDasherX = xmap(iDasherX);
	iDasherY = ymap(iDasherY);

	// Things we're likely to need:

	screenint iScreenWidth = Screen()->GetWidth();
	screenint iScreenHeight = Screen()->GetHeight();

	// Note that integer division is rounded *away* from zero here to
	// ensure that this really is the inverse of the map the other way
	// around.

	switch (GetOrientation())
	{
	case Options::LeftToRight:
		iScreenX = static_cast<screenint>(iScreenWidth - CustomIDivScaleFactor(iDasherX * iScaleFactorX));
		iScreenY = static_cast<screenint>(iScreenHeight / 2 + CustomIDivScaleFactor((iDasherY - CDasherModel::MAX_Y / 2) * iScaleFactorY));
		break;
	case Options::RightToLeft:
		iScreenX = static_cast<screenint>(CustomIDivScaleFactor(iDasherX * iScaleFactorX));
		iScreenY = static_cast<screenint>(iScreenHeight / 2 + CustomIDivScaleFactor((iDasherY - CDasherModel::MAX_Y / 2) * iScaleFactorY));
		break;
	case Options::TopToBottom:
		iScreenX = static_cast<screenint>(iScreenWidth / 2 + CustomIDivScaleFactor((iDasherY - CDasherModel::MAX_Y / 2) * iScaleFactorY));
		iScreenY = static_cast<screenint>(iScreenHeight - CustomIDivScaleFactor(iDasherX * iScaleFactorX));
		break;
	case Options::BottomToTop:
		iScreenX = static_cast<screenint>(iScreenWidth / 2 + CustomIDivScaleFactor((iDasherY - CDasherModel::MAX_Y / 2) * iScaleFactorY));
		iScreenY = static_cast<screenint>(CustomIDivScaleFactor(iDasherX * iScaleFactorX));
		break;
	default:
		break;
	}
}

void CDasherViewSquare::Dasher2Polar(myint iDasherX, myint iDasherY, double& r, double& theta)
{
	iDasherX = xmap(iDasherX);
	iDasherY = ymap(iDasherY);

	const myint iDasherOX = xmap(CDasherModel::ORIGIN_X);
	const myint iDasherOY = ymap(CDasherModel::ORIGIN_Y);

	double x = -(iDasherX - iDasherOX) / static_cast<double>(iDasherOX); //Use normalised coords so min r works
	double y = -(iDasherY - iDasherOY) / static_cast<double>(iDasherOY);
	theta = atan2(y, x);
	r = sqrt(x * x + y * y);
}

void CDasherViewSquare::DasherLine2Screen(myint x1, myint y1, myint x2, myint y2, std::vector<point>& vPoints)
{
	if (x1 != x2 && y1 != y2)
	{
		//only diagonal lines ever get changed...
		if (m_pSettingsStore->GetBoolParameter(BP_NONLINEAR_Y))
		{
			if ((y1 < m_Y3 && y2 > m_Y3) || (y2 < m_Y3 && y1 > m_Y3))
			{
				//crosses bottom non-linearity border
				myint x_mid = x1 + (x2 - x1) * (m_Y3 - y1) / (y2 - y1);
				DasherLine2Screen(x1, y1, x_mid, m_Y3, vPoints);
				x1 = x_mid;
				y1 = m_Y3;
			} //else //no, a single line might cross _both_ borders!
			if ((y1 > m_Y2 && y2 < m_Y2) || (y2 > m_Y2 && y1 < m_Y2))
			{
				//crosses top non-linearity border
				myint x_mid = x1 + (x2 - x1) * (m_Y2 - y1) / (y2 - y1);
				DasherLine2Screen(x1, y1, x_mid, m_Y2, vPoints);
				x1 = x_mid;
				y1 = m_Y2;
			}
		}
		if (m_pSettingsStore->GetLongParameter(LP_NONLINEAR_X) && (x1 > m_iXlogThres || x2 > m_iXlogThres))
		{
			//into logarithmic section
			point pStart, pScreenMid, pEnd;
			Dasher2Screen(x2, y2, pEnd.x, pEnd.y);
			for (;;)
			{
				Dasher2Screen(x1, y1, pStart.x, pStart.y);
				//a straight line on the screen between pStart and pEnd passes through pScreenMid:
				pScreenMid.x = (pStart.x + pEnd.x) / 2;
				pScreenMid.y = (pStart.y + pEnd.y) / 2;
				//whereas a straight line _in_Dasher_space_ passes through pDasherMid:
				myint xMid = (x1 + x2) / 2, yMid = (y1 + y2) / 2;
				point pDasherMid;
				Dasher2Screen(xMid, yMid, pDasherMid.x, pDasherMid.y);

				//since we know both endpoints are in the same section of the screen wrt. Y nonlinearity,
				//the midpoint along the DasherY axis of both lines should be the same.
				if (GetOrientation() == Options::LeftToRight || GetOrientation() == Options::RightToLeft)
				{
					DASHER_ASSERT(abs(pDasherMid.y - pScreenMid.y) <= 1); //allow for rounding error
					if (abs(pDasherMid.x - pScreenMid.x) <= 1) break; //call a straight line accurate enough
				}
				else
				{
					DASHER_ASSERT(abs(pDasherMid.x - pScreenMid.x) <= 1);
					if (abs(pDasherMid.y - pScreenMid.y) <= 1) break;
				}
				//line should appear bent. Subdivide!
				DasherLine2Screen(x1, y1, xMid, yMid, vPoints); //recurse for first half (to Dasher-space midpoint)
				if (x1 == xMid || y1 == yMid) break; // as test on entry, only diagonal lines need to be bent...
				x1 = xMid;
				y1 = yMid; //& loop round for second half
			}
			//broke out of loop. a straight line (x1,y1)-(x2,y2) on the screen is an accurate portrayal of a straight line in Dasher-space.
			vPoints.push_back(pEnd);
			return;
		}
		//ok, not in x nonlinear section; fall through.
	}

	point p;
	Dasher2Screen(x2, y2, p.x, p.y);
	vPoints.push_back(p);
}

CDasherView::DasherCoordScreenRegion CDasherViewSquare::VisibleRegion()
{
	if (!m_bVisibleRegionValid)
	{
		switch (GetOrientation())
		{
		case Options::LeftToRight:
			Screen2Dasher(Screen()->GetWidth(), 0, m_visible_region.minX, m_visible_region.minY);
			Screen2Dasher(0, Screen()->GetHeight(), m_visible_region.maxX, m_visible_region.maxY);
			break;
		case Options::RightToLeft:
			Screen2Dasher(0, 0, m_visible_region.minX, m_visible_region.minY);
			Screen2Dasher(Screen()->GetWidth(), Screen()->GetHeight(), m_visible_region.maxX, m_visible_region.maxY);
			break;
		case Options::TopToBottom:
			Screen2Dasher(0, Screen()->GetHeight(), m_visible_region.minX, m_visible_region.minY);
			Screen2Dasher(Screen()->GetWidth(), 0, m_visible_region.maxX, m_visible_region.maxY);
			break;
		case Options::BottomToTop:
			Screen2Dasher(0, 0, m_visible_region.minX, m_visible_region.minY);
			Screen2Dasher(Screen()->GetWidth(), Screen()->GetHeight(), m_visible_region.maxX, m_visible_region.maxY);
			break;
		default:
			break;
		}

		m_bVisibleRegionValid = true;
	}

	return {m_visible_region.minX, m_visible_region.minY, m_visible_region.maxX, m_visible_region.maxY};
}

void CDasherViewSquare::ScreenResized(CDasherScreen* NewScreen)
{
	m_bVisibleRegionValid = false;
	SetScaleFactor();
}

/// Draw the crosshair

inline void CDasherViewSquare::Crosshair()
{
	const DasherCoordScreenRegion visibleRegion = VisibleRegion();

	myint x[2];
	myint y[2];

	// Vertical bar of crosshair

	x[0] = CDasherModel::ORIGIN_X;
	y[0] = visibleRegion.minY;

	x[1] = CDasherModel::ORIGIN_X;
	y[1] = visibleRegion.maxY;

	DasherPolyline(x, y, 2, 1, GetNamedColor(NamedColor::crosshair));

	// Horizontal bar of crosshair

	x[0] = 12 * CDasherModel::ORIGIN_X / 14;
	y[0] = CDasherModel::ORIGIN_Y;

	x[1] = 17 * CDasherModel::ORIGIN_X / 14;
	y[1] = CDasherModel::ORIGIN_Y;

	DasherPolyline(x, y, 2, 1, GetNamedColor(NamedColor::crosshair));
}

inline myint CDasherViewSquare::ixmap(myint x) const
{
	x -= iMarginWidth;
	if (m_pSettingsStore->GetLongParameter(LP_NONLINEAR_X) > 0 && x >= m_iXlogThres)
	{
		double dx = (x - m_iXlogThres) / static_cast<double>(CDasherModel::MAX_Y);
		dx = (exp(dx * m_dXlogCoeff) - 1) / m_dXlogCoeff;
		x = myint(dx * CDasherModel::MAX_Y) + m_iXlogThres;
	}
	return x;
}

inline myint CDasherViewSquare::xmap(myint x) const
{
	if (m_pSettingsStore->GetLongParameter(LP_NONLINEAR_X) && x >= m_iXlogThres)
	{
		double dx = log(1 + (x - m_iXlogThres) * m_dXlogCoeff / CDasherModel::MAX_Y) / m_dXlogCoeff;
		dx = (dx * CDasherModel::MAX_Y) + m_iXlogThres;
		x = myint(dx > 0 ? ceil(dx) : floor(dx));
	}
	return x + iMarginWidth;
}

inline myint CDasherViewSquare::ymap(myint y) const
{
	if (m_pSettingsStore->GetBoolParameter(BP_NONLINEAR_Y))
	{
		if (y > m_Y2)
			return m_Y2 + (y - m_Y2) / m_Y1;
		else if (y < m_Y3)
			return m_Y3 + (y - m_Y3) / m_Y1;
	}
	return y;
}

inline myint CDasherViewSquare::iymap(myint ydash) const
{
	if (m_pSettingsStore->GetBoolParameter(BP_NONLINEAR_Y))
	{
		if (ydash > m_Y2)
			return (ydash - m_Y2) * m_Y1 + m_Y2;
		else if (ydash < m_Y3)
			return (ydash - m_Y3) * m_Y1 + m_Y3;
	}
	return ydash;
}
