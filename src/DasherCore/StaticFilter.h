#pragma once

#include "InputFilter.h"
#include "SettingsStore.h"

namespace Dasher {
/// Simple class, basis for filters using ScheduleZoom rather than
///  ScheduleOneStep, providing a ScheduleZoom method wrapping the
///  DasherModel one, using LP_ZOOMSTEPS steps and such that pause()
///  cancels any such zoom in progress.
class CStaticFilter : public CInputFilter {
  public:
    CStaticFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pIntf, const char* szName);
    void pause() override;

    virtual void GetUISettings(std::vector<Dasher::Parameter>& List) override;

  protected:
    void ScheduleZoom(CDasherModel* pModel, myint y1, myint y2);
    CSettingsStore* m_pSettingsStore;

  private:
    CDasherModel* m_pModel;
};
} // namespace Dasher
/// @}