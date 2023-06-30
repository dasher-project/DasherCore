#include "PressFilter.h"

Dasher::CPressFilter::CPressFilter(CSettingsUser* pCreator, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate) : CDefaultFilter(pCreator, pInterface, pFramerate, 2, "Press Mode")
{
}

void Dasher::CPressFilter::KeyDown(unsigned long iTime, int iId, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((iId == 0 && GetBoolParameter(BP_START_SPACE))
		|| (iId == 100 && GetBoolParameter(BP_START_MOUSE)))
	{
		run(iTime);
	}
	else if (iId == 101 || iId == 102 || iId == 1)
	{
		//Other mouse buttons, if platforms support; or button 1
		if (GetBoolParameter(BP_TURBO_MODE)) m_bTurbo = true;
	}
}

void Dasher::CPressFilter::KeyUp(unsigned long iTime, int iId, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((iId == 0 && GetBoolParameter(BP_START_SPACE)) || (iId == 100 && GetBoolParameter(BP_START_MOUSE)))
	{
		stop();
	}
	else if (iId == 101 || iId == 102 || iId == 1) m_bTurbo = false;
}
