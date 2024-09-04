#include "SmoothingFilter.h"

#include <complex>

Dasher::CSmoothingFilter::CSmoothingFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate, const char *szName) : CPressFilter(pSettingsStore, pInterface, pFramerate, szName)
{
}

void Dasher::CSmoothingFilter::KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pDasherView, CDasherInput* pInput, CDasherModel* pModel)
{
	if (m_pSettingsStore->GetBoolParameter(BP_SMOOTH_PRESS_MODE))
	{
		CPressFilter::KeyDown(iTime, Key, pDasherView, pInput, pModel);
	}
	else
	{
		CDefaultFilter::KeyDown(iTime, Key, pDasherView, pInput, pModel);
	}
}

void Dasher::CSmoothingFilter::KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel)
{
	if (m_pSettingsStore->GetBoolParameter(BP_SMOOTH_PRESS_MODE))
	{
		CPressFilter::KeyUp(iTime, Key, pView, pInput, pModel);
	}
	else
	{
		CDefaultFilter::KeyUp(iTime, Key, pView, pInput, pModel);
	}
}

void Dasher::CSmoothingFilter::Timer(unsigned long Time, CDasherView* pView, CDasherInput* pInput, CDasherModel* pModel,
    CExpansionPolicy** pol)
{
    myint newX, newY;
    if (!pInput->GetDasherCoords(newX, newY, pView))
    {
        m_bGotMouseCoords = false;
        stop();
        return;
    }

	myint originX = CDasherModel::ORIGIN_X;
	myint originY = CDasherModel::ORIGIN_Y;
	pView->ClipLineToVisible(originX, originY, newX, newY);
	lastCursorPosX = newX;
	lastCursorPosY = newY;

	currentlySmoothing = newX < CDasherModel::ORIGIN_X || !m_pSettingsStore->GetBoolParameter(BP_SMOOTH_ONLY_FORWARD);
	if(currentlySmoothing)
	{
	    const float smoothingAlpha = std::max(std::min(
										    1.0f - std::exp(-static_cast<float>(Time - LastUpdate) / static_cast<float>(m_pSettingsStore->GetLongParameter(LP_SMOOTH_TAU))),
									    1.0f),0.0f);
	        
	    SmoothedPositionX = smoothingAlpha * static_cast<float>(newX) + (1.0f - smoothingAlpha) * SmoothedPositionX;
	    SmoothedPositionY = smoothingAlpha * static_cast<float>(newY) + (1.0f - smoothingAlpha) * SmoothedPositionY;
	}
    else
	{
	    SmoothedPositionX = static_cast<float>(newX);
	    SmoothedPositionY = static_cast<float>(newY);
	}

	LastUpdate = Time;
	ExecuteMovement(Time, pView, pModel, static_cast<screenint>(SmoothedPositionX), static_cast<screenint>(SmoothedPositionY));
}

bool Dasher::CSmoothingFilter::DecorateView(CDasherView* pView, CDasherInput* pInput)
{
	bool result = false;

	if (currentlySmoothing && m_pSettingsStore->GetBoolParameter(BP_SMOOTH_DRAW_MOUSE))
    {
        //Draw a small box at the current mouse position
        pView->DasherDrawCentredRectangle(lastCursorPosX, lastCursorPosY, 5, pView->GetNamedColor(NamedColor::inputPosition).lerp(ColorPalette::white, 0.75f),
                                          ColorPalette::noColor, false);
		result = true;
    }

	if (currentlySmoothing && m_pSettingsStore->GetBoolParameter(BP_SMOOTH_DRAW_MOUSE_LINE))
    {
		myint p[4] = {m_iLastX, lastCursorPosX, m_iLastY, lastCursorPosY};
        //Draw a small box at the current mouse position
        pView->DasherPolyline(&p[0], &p[2], 2, m_pSettingsStore->GetLongParameter(LP_LINE_WIDTH),
                                  pView->GetNamedColor(NamedColor::inputLine).lerp(ColorPalette::white, 0.75f));
		result = true;
    }


	result = CPressFilter::DecorateView(pView, pInput) | result;
    return result;
}
