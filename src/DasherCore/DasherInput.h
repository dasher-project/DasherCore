// DasherInput.h
//
// Copyright (c) 2005 Phil Cowans

#pragma once

#include "DasherTypes.h"
#include "DasherModule.h"
#include "ModuleManager.h"
#include "DasherView.h"
#include "DasherModel.h"

namespace Dasher {
class CDasherInput;
class CDasherCoordInput;
class CScreenCoordInput;
class CDasherVectorInput;
class CDasherInterfaceBase;
} // namespace Dasher
/// \defgroup Input Input devices
/// \{
class Dasher::CDasherInput : public CDasherModule {

  public:
    CDasherInput(const char* szName) : CDasherModule(szName) {};

    /// Get the position of the input in Dasher coordinates.
    /// \param iDasherX X-coordinate; if only one coordinate (axis) is available, this should be 0.
    /// \param iDasherY Y-coordinate; if only one coordinate (axis) is available, this should be it.
    /// \param pView view to use to convert Screen2Dasher, if necessary
    /// \return true if coordinates were obtained; false if they could not be.
    virtual bool GetDasherCoords(myint& iDasherX, myint& iDasherY, CDasherView* pView) = 0;

    /// Get the position of the input in screen coordinates. If only one coordinate (axis) is available,
    /// it should be returned in Y for left-to-right / right-to-left orientations, X for top-to-bottom/
    /// bottom-to-top. (As would happen if performing Dasher2Screen on values obtained from GetDasherCoords!)
    /// \param iX Screen X-coordinate
    /// \param iY Screen Y-coordinate
    /// \param pView view to use to convert Dasher2Screen, if necessary
    /// \return true if coordinates were obtained; false if they could not be.
    virtual bool GetScreenCoords(screenint& iX, screenint& iY, CDasherView* pView) = 0;

    /// Activate the device. If a helper thread needs to be started in
    /// order to listen for input then do it here.
    virtual void Activate() {};

    /// Deactivate the device. Please don't hold on to any significant
    /// resources (eg helper threads) after deactivation.
    virtual void Deactivate() {};

    /// Handle key down events
    ///
    virtual void KeyDown(unsigned long iTime, int iId) {};

    ///
    /// Handle key up events
    virtual void KeyUp(unsigned long iTime, int iId) {};
};
/// Abstract superclasses for CDasherInputs which natively provide screen coordinates;
///  thus, when dasher-coordinates are requested, they will be converted
///  via the views Screen2Dasher. (=>Subclasses must implement GetScreenCoords)
class Dasher::CScreenCoordInput : public CDasherInput {
  public:
    CScreenCoordInput(const char* szName) : CDasherInput(szName) {}
    virtual bool GetDasherCoords(myint& iDasherX, myint& iDasherY, CDasherView* pView) {
        screenint iX, iY;
        if (!GetScreenCoords(iX, iY, pView)) return false;
        pView->Screen2Dasher(iX, iY, iDasherX, iDasherY);
        return true;
    }
};

/// Abstract superclasses for CDasherInputs which natively provide dasher coordinates;
///  thus, when screen-coordinates are requested, they will be converted
///  via the views Dasher2Screen. (=>Subclasses must implement GetDasherCoords)
class Dasher::CDasherCoordInput : public CDasherInput {
  public:
    CDasherCoordInput(const char* szName) : CDasherInput(szName) {}

    virtual bool GetScreenCoords(screenint& iX, screenint& iY, CDasherView* pView) {
        myint iDasherX, iDasherY;
        if (!GetDasherCoords(iDasherX, iDasherY, pView)) return false;
        pView->Dasher2Screen(iDasherX, iDasherY, iX, iY);
        return true;
    }
};

/// \}

/// Abstract superclasses for CDasherInputs which natively provides [0,1] inputs in screenspace (not subject to
/// non-linearity); Coordinate system spans from bottom left (-1,-1) to top right (1,1)
/// (=> Subclasses must implement GetVectorCoords)
class Dasher::CDasherVectorInput : public Dasher::CScreenCoordInput {
  public:
    CDasherVectorInput(const char* szName) : CScreenCoordInput(szName) {}

    virtual bool GetVectorCoords(float& VectorX, float& VectorY) = 0;

    virtual bool GetScreenCoords(screenint& iX, screenint& iY, CDasherView* pView) {
        float inputX, inputY;
        if (!GetVectorCoords(inputX, inputY)) return false;

        Dasher::screenint originX, originY;
        pView->Dasher2Screen(CDasherModel::ORIGIN_X, CDasherModel::ORIGIN_Y, originX, originY);

        const CDasherView::ScreenRegion vR = {0, 0, pView->Screen()->GetWidth(), pView->Screen()->GetHeight()};
        const myint top = vR.minY - originY;
        const myint bottom = vR.maxY - originY;
        const myint left = vR.minX - originX;
        const myint right = vR.maxX - originX;

        iX = ((inputX > 0) ? right * inputX : left * -inputX) + originX;
        iY = ((inputY > 0) ? top * inputY : bottom * -inputY) + originY;

        return true;
    }
};
