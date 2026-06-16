#pragma once

#include "DynamicFilter.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
class CDemoFilter : public CDynamicFilter {
  public:
    CDemoFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface, CFrameRate* pFramerate);
    virtual ~CDemoFilter();

    virtual void HandleEvent(Parameter parameter);

    virtual bool DecorateView(CDasherView* pView, CDasherInput* pInput) override;
    virtual void Timer(unsigned long Time, CDasherView* m_pDasherView, CDasherInput* pInput,
                       CDasherModel* m_pDasherModel, CExpansionPolicy** pol) override;
    virtual void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pDasherView, CDasherInput* pInput,
                         CDasherModel* pModel) override;
    virtual void Activate() override;
    virtual void Deactivate() override;

    virtual void GetUISettings(std::vector<Dasher::Parameter>& List) override;

  private:
    double m_dSpring, m_dNoiseNew, m_dNoiseOld;
    double m_dNoiseX, m_dNoiseY;
    myint m_iDemoX, m_iDemoY;
};
} // namespace Dasher
/// @}
