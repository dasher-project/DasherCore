// DasherViewSquare.h
//
// Copyright (c) 2001-2004 David Ward

#pragma once

#include "DasherModel.h"
#include "DasherView.h"
#include "DasherScreen.h"
#include "SettingsStore.h"


namespace Dasher
{
class CDasherViewSquare;
class CDasherView;
class CDasherNode;
}

/// \ingroup View
/// @{

/// An implementation of the DasherView class
///
/// This class renders Dasher in the vanilla style,
/// but with horizontal and vertical mappings
///
/// Horizontal mapping - linear and log
/// Vertical mapping - linear with different gradient
class Dasher::CDasherViewSquare : public Dasher::CDasherView, public CSettingsUserObserver
{
public:
	/// Constructor
	///
	/// \param DasherScreen Pointer to screen to which the view will render.
	/// \todo Don't cache screen and model locally - screen can be
	/// passed as parameter to the drawing functions, and data structure
	/// can be extracted from the model and passed too.

	CDasherViewSquare(CSettingsUser* pCreateFrom, CDasherScreen* DasherScreen, Options::ScreenOrientations orient);
	~CDasherViewSquare() override;

	///
	/// Event handler
	///

	void HandleEvent(Parameter parameter) override;

	//Override to additionally reset scale factors etc.
	void SetOrientation(Options::ScreenOrientations newOrient) override;

	/// Resets scale factors etc. that depend on the screen size, to be recomputed when next needed.
	void ScreenResized(CDasherScreen* NewScreen) override;

	///
	/// @name Coordinate system conversion
	/// Convert between screen and Dasher coordinates
	/// @{

	///
	/// Convert a screen co-ordinate to Dasher co-ordinates
	///
	void Screen2Dasher(screenint iInputX, screenint iInputY, myint& iDasherX, myint& iDasherY) override;

	///
	/// Convert Dasher co-ordinates to screen co-ordinates
	///
	void Dasher2Screen(myint iDasherX, myint iDasherY, screenint& iScreenX, screenint& iScreenY) override;

	///
	/// Convert Dasher co-ordinates to polar co-ordinates (r,theta), with 0<r<1, 0<theta<2*pi
	///
	void Dasher2Polar(myint iDasherX, myint iDasherY, double& r, double& theta) override;

	///
	/// Return true if there is any space around a node spanning y1 to y2
	/// and the screen boundary; return false if such a node entirely encloses
	/// the screen boundary
	///
	bool IsSpaceAroundNode(myint y1, myint y2) override;

	///
	/// Get the bounding box of the visible region.
	///
	ScreenRegion VisibleRegion() override;

	///
	/// Render all nodes, inc. blanking around the root (supplied)
	///
	CDasherNode* Render(CDasherNode* pRoot, myint iRootMin, myint iRootMax, CExpansionPolicy& policy) override;

	/// @}

	void DasherSpaceArc(myint cy, myint r, myint x1, myint y1, myint x2, myint y2, const ColorPalette::Color& color, int iLineWidth) override;

private:
	///draw a possibly-truncated triangle given dasher-space coords & accounting for non-linearity
	/// @param x = max dasher-x extent
	/// @param y1, y2 = dasher-y extent along y-axis
	/// @param midy1,midy2 = extent along line of max x (midy1==midy2 => triangle, midy1<midy2 => truncated tri)
	void TruncateTri(myint x, myint y1, myint y2, myint midy1, myint midy2, const ColorPalette::Color& fillColor, const ColorPalette::Color&
                     outlineColor, int lineWidth);

	/// compute screen coords for a circle, centered on y-axis, between two points
	/// cy, r - dasher coords of center (on y-axis), radius
	/// x1,y1 - one end-point of arc (dasher coords)
	/// x2,y2 - other end-point (dasher-coords)
	/// dest - point (x2,y2) in screen coords
	/// pts - vector into which to store points; on entry, last element should already be screen-coords of (x1,y1)
	/// dXMul - multiply x coords (in dasher space) by this (i.e. aspect ratio), for ovals
	void CircleTo(myint cy, myint r, myint y1, myint x1, myint y3, myint x3, CDasherScreen::point dest, std::vector<CDasherScreen::point>& pts, double dXMul);
	void Circle(myint Range, myint y1, myint y2, const ColorPalette::Color& fillColor, const ColorPalette::Color& outlineColor, int lineWidth);
	void Quadric(myint Range, myint lowY, myint highY, const ColorPalette::Color& fillColor, const ColorPalette::Color& outlineColor, int
                 lineWidth);
	///draw isoceles triangle, with baseline from y1-y2 along y axis (x=0), and other point at (x,(y1+y2)/2)
	/// (all in Dasher coords).
	void Triangle(myint x, myint y1, myint y2, int fillColor, int outlineColor, int lineWidth);

	class CTextString
	{
	public: //to CDasherViewSquare...
		///Creates a request that label will be drawn.
		/// x,y are screen coords of midpoint of leading edge;
		/// iSize is desired size (already computed from requested position)
		CTextString(CDasherScreen::Label* pLabel, screenint x, screenint y, int iSize, int iColor)
			: m_pLabel(pLabel), m_ix(x), m_iy(y), m_iSize(iSize), m_iColor(iColor)
		{
		}

		~CTextString();
		CDasherScreen::Label* m_pLabel;
		screenint m_ix, m_iy;
		std::vector<CTextString*> m_children;
		int m_iSize;
		int m_iColor;
	};

	std::vector<CTextString*> m_DelayedTexts;
	//ExtrusionLevel is used for 3DRendering
	void DoDelayedText(CTextString* pText, myint extrusionLevel = 0, myint groupRecursionDepth = 0);

	struct geometry_3DText
	{
		CTextString* root_node;
		myint extrusionLevel;
		myint groupRecursionDepth;
	};
	std::vector<geometry_3DText> m_Delayed3DTexts;
	myint m_CrosshairCubeLevel = -1;

	///
	/// Draw text specified in Dasher co-ordinates
	///
	CTextString* DasherDrawText(myint iDasherMaxX, myint iDasherMidY, CDasherScreen::Label* pLabel, int iColor);

	///
	/// (Recursively) render a node and all contained subnodes, in disjoint rects.
	/// (i.e. appropriate for LP_SHAPE_TYPE==0). Each call responsible for rendering
	/// exactly the area contained within the node.
	/// @param pOutput The innermost node covering the crosshair (if any)
	void DisjointRender(CDasherNode* Render, myint y1, myint y2, CTextString* prevText, CExpansionPolicy& policy, double dMaxCost, CDasherNode*& pOutput);

	void DasherDrawCube(myint iDasherMaxX, myint iDasherMinY, myint iDasherMinX, myint iDasherMaxY, myint extrusionLevel, myint
                        groupRecursionDepth, const ColorPalette::Color& Color, const ColorPalette::Color& outlineColor, int iThickness);
	/// (Recursively) render a node and all contained subnodes, in overlapping shapes
	/// (according to LP_SHAPE_TYPE)
	/// Each call responsible for rendering exactly the area contained within the node.
	/// @param pCurrentTopCenterNode The innermost node covering the crosshair (if any)
	void NewRender(CDasherNode* pCurrentNode, myint y1, myint y2, CTextString* pPrevText, CExpansionPolicy& policy, double dMaxCost, CDasherNode*& pCurrentTopCenterNode, myint recusionDepth, myint groupRecursionDepth);

	/// @name Nonlinearity
	/// Implements the non-linear part of the coordinate space mapping

	/// Maps a dasher coordinate (linear in probability space, -ive x = in margin) to an abstract/resolution-independent
	/// screen coordinate (linear in screen space, -ive x = offscreen) - i.e. pixel coordinate = scale({x,y}map(dasher coord)))
	inline myint ymap(myint iDasherY) const;
	inline myint xmap(myint iDasherX) const;

	/// Inverse of the previous - i.e. dasher coord = iymap(scale(screen coord))
	inline myint iymap(myint y) const;
	inline myint ixmap(myint x) const;

	///Parameters for y non-linearity. (TODO Make into preprocessor defines?)
	const myint m_Y1 = 4;
	const myint m_Y2 = static_cast<const myint>(0.95 * CDasherModel::MAX_Y);
	const myint m_Y3 = static_cast<const myint>(0.05 * CDasherModel::MAX_Y);

	inline void Crosshair();
	bool CoversCrosshair(myint Range, myint y1, myint y2);
    ColorPalette::Color SimulateTransparency(CDasherNode* pCurrentNode);

    //Divides by SCALE_FACTOR, rounding away from 0
	inline myint CustomIDivScaleFactor(myint iNumerator);

	void DasherLine2Screen(myint x1, myint y1, myint x2, myint y2, std::vector<CDasherScreen::point>& vPoints) override;

	// Called on screen size or orientation changes
	void SetScaleFactor();

	// Parameters for x non-linearity
	double m_dXlogCoeff;
	myint m_iXlogThres;

	//width of margin, in abstract screen coords
	myint iMarginWidth;

	/// There is a ratio of iScaleFactor{X,Y} abstract screen coords to SCALE_FACTOR real pixels
	/// (Note the naming convention: iScaleFactorX/Y refers to X/Y in Dasher-space, which will be
	/// the other way around to real screen coordinates if using a vertical (T-B/B-T) orientation)
	myint iScaleFactorX, iScaleFactorY;
	static const myint SCALE_FACTOR = 1 << 26; //was 100,000,000; change to power of 2 => easier to multiply/divide

	bool m_bVisibleRegionValid = false;
	ScreenRegion m_visible_region;
};

/// @}
