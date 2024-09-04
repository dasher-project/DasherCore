#pragma once
#include "PressFilter.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
///	Small adaption of Default Filter to only move while mouse-button is pressed
class CSmoothingFilter : public CPressFilter {
public:
	CSmoothingFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate, const char *szName);
	void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel) override;
	void KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel) override;
    void Timer(unsigned long Time, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel, CExpansionPolicy** pol) override;
    bool DecorateView(CDasherView* pView, CDasherInput* pInput) override;

private:
    float SmoothedPositionX = -1.0f;
    float SmoothedPositionY = -1.0f;
    myint lastCursorPosX, lastCursorPosY = 0;
    bool currentlySmoothing = false;
    unsigned long LastUpdate = -1;

};
/// @}
}
