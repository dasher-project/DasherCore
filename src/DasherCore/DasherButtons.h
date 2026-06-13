
// DasherButtons.h
// Copyright 2005 by Chris Ball

#pragma once

#include "SettingsStore.h"
#include "StaticFilter.h"

namespace Dasher {
/// \ingroup Input
/// @{
class CDasherButtons : public CStaticFilter {
  public:
    CDasherButtons(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface, bool bMenu, const char* szName);
    virtual ~CDasherButtons();

    virtual bool DecorateView(CDasherView* pView, CDasherInput* pInput) override = 0;

    void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput,
                 CDasherModel* pModel) override;
    void Timer(unsigned long Time, CDasherView* pView, CDasherInput* pInput, CDasherModel* m_pDasherModel,
               CExpansionPolicy** pol) override;
    void Activate() override;

    virtual void GetUISettings(std::vector<Dasher::Parameter>& List) override;

    struct SBoxInfo {
        int iTop;
        int iBottom;
        int iDisplayTop;
        int iDisplayBottom;
    };

  protected:
    virtual void SetupBoxes() = 0;
    void NewDrawGoTo(CDasherView* pView, myint iDasherMin, myint iDasherMax, bool bActive);
    bool m_bMenu;
    bool m_bDecorationChanged;
    SBoxInfo* m_pBoxes;
    int m_iNumBoxes, iActiveBox;
    unsigned long m_iScanTime;

    virtual void DirectKeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherModel* pModel);
};
} // namespace Dasher
/// @}
