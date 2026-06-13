#pragma once

#include "StaticFilter.h"
#include "SettingsStore.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
class COneButtonFilter : public CStaticFilter {
  public:
    COneButtonFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface);

    virtual bool DecorateView(CDasherView* pView, CDasherInput* pInput) override;
    virtual void Timer(unsigned long Time, CDasherView* pView, CDasherInput* pInput, CDasherModel* m_pDasherModel,
                       CExpansionPolicy** pol) override;
    virtual void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherInput* pInput,
                         CDasherModel* pModel) override;
    bool GetSettings(SModuleSettings** pSettings, int* iCount) override;
    virtual void GetUISettings(std::vector<Dasher::Parameter>& List) override;

  private:
    /// true iff the scan line is moving down/up, or is in the 'reverse' stage
    bool bStarted;
    /// set by DecorateView: true iff we have drawn an undecorated display, else false.
    bool m_bNoDecorations;
    unsigned long iStartTime;
    int iLocation;
};
} // namespace Dasher
/// @}
