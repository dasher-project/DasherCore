#include "PressFilter.h"

Dasher::CPressFilter::CPressFilter(CSettingsUser* pCreator, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate) : CDefaultFilter(pCreator, pInterface, pFramerate, 2, "Press Mode")
{
}

void Dasher::CPressFilter::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((Key == Keys::Big_Start_Stop_Key && GetBoolParameter(BP_START_SPACE))
		|| (Key == Keys::Primary_Input && GetBoolParameter(BP_START_MOUSE)))
	{
		run(iTime);
	}
	else if (Key == Keys::Secondary_Input || Key == Keys::Tertiary_Input || Key == Keys::Button_1)
	{
		//Other mouse buttons, if platforms support; or button 1
		if (GetBoolParameter(BP_TURBO_MODE)) m_bTurbo = true;
	}
}

void Dasher::CPressFilter::KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((Key == Keys::Big_Start_Stop_Key && GetBoolParameter(BP_START_SPACE)) || (Key == Keys::Primary_Input && GetBoolParameter(BP_START_MOUSE)))
	{
		stop();
	}
	else if (Key == Keys::Secondary_Input || Key == Keys::Tertiary_Input || Key == Keys::Button_1) m_bTurbo = false;
}
