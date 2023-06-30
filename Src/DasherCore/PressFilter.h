#pragma once
#include "DefaultFilter.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
///	Small adaption of Default Filter to only move while mouse-button is pressed
class CPressFilter : public CDefaultFilter {
public:
	CPressFilter(CSettingsUser* pCreator, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate);
	void KeyDown(unsigned long iTime, int iId, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel) override;
	void KeyUp(unsigned long iTime, int iId, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel) override;
};
/// @}
}