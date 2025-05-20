#include "PressFilter.h"
#include "I18n.h"

Dasher::CPressFilter::CPressFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate, const char *szName) : CDefaultFilter(pSettingsStore, pInterface, pFramerate, szName)
{
	UIList.push_back(Dasher::Settings::SwitchSetting(BP_REMAP_XTREME, _("At top and bottom, scroll more and translate less (makes error-correcting easier)")));
    UIList.push_back(Dasher::Settings::EnumSetting(LP_GEOMETRY, "Screen geometry", {
        {"Old Style", Dasher::Options::ScreenGeometry::old_style},
        {"Square without Crosshair", Dasher::Options::ScreenGeometry::square_no_xhair},
        {"Squish", Dasher::Options::ScreenGeometry::squish},
        {"Squaish + Log", Dasher::Options::ScreenGeometry::squish_and_log},
    }));
    UIList.push_back(Dasher::Settings::EnumSetting(LP_SHAPE_TYPE, "Rendering Geometry", {
        {"Rectangle", Dasher::Options::RenderingShapeTypes::OVERLAPPING_RECTANGLE},
        {"Triangle", Dasher::Options::RenderingShapeTypes::TRIANGLE},
        {"Truncated Triangle", Dasher::Options::RenderingShapeTypes::TRUNCATED_TRIANGLE},
        {"Quadric", Dasher::Options::RenderingShapeTypes::QUADRIC},
        {"Circle", Dasher::Options::RenderingShapeTypes::CIRCLE}
    }));
}

void Dasher::CPressFilter::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((Key == Keys::Big_Start_Stop_Key && m_pSettingsStore->GetBoolParameter(BP_START_SPACE))
		|| (Key == Keys::Primary_Input && m_pSettingsStore->GetBoolParameter(BP_START_MOUSE)))
	{
		run(iTime);
	}
	else if (Key == Keys::Secondary_Input || Key == Keys::Tertiary_Input || Key == Keys::Button_1)
	{
		//Other mouse buttons, if platforms support; or button 1
		if (m_pSettingsStore->GetBoolParameter(BP_TURBO_MODE)) m_bTurbo = true;
	}
}

void Dasher::CPressFilter::KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel)
{
	if ((Key == Keys::Big_Start_Stop_Key && m_pSettingsStore->GetBoolParameter(BP_START_SPACE)) || (Key == Keys::Primary_Input && m_pSettingsStore->GetBoolParameter(BP_START_MOUSE)))
	{
		stop();
	}
	else if (Key == Keys::Secondary_Input || Key == Keys::Tertiary_Input || Key == Keys::Button_1) m_bTurbo = false;
}