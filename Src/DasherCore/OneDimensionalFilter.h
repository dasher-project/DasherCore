#pragma once

#include <I18n.h>

#include "CircleStartHandler.h"
#include "DefaultFilter.h"

/// \ingroup InputFilter
/// @{
namespace Dasher {
    class C1DCircleStartHandler;



class COneDimensionalFilter : public CDefaultFilter {
 public:
//  COneDimensionalFilter(CSettingsStore *pSettingsStore, CDasherInterfaceBase *pInterface, CDasherModel *m_pDasherModel);
  COneDimensionalFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName = _("One Dimensional Mode"));
  ///Override to remove DefaultFilters BP_REMAP_XTREME, BP_AUTOCALIBRATE, LP_OFFSET
  bool GetSettings(SModuleSettings **pSettings, int *iCount);
 protected:
  friend C1DCircleStartHandler;
  virtual void ApplyTransform(myint &iDasherX, myint &iDasherY, CDasherView *pView);
  const myint forwardmax;
  virtual CStartHandler *MakeStartHandler();
};

class C1DCircleStartHandler : public CCircleStartHandler {
public:
  C1DCircleStartHandler(COneDimensionalFilter *f, CSettingsStore* pSettingsStore);
  CDasherScreen::point CircleCenter(CDasherView *pView) override;

  void onPause() override;

private:
  CDasherScreen::point m_fwdCenter;
  CSettingsStore* m_pSettingsStore;
};
}
/// @}

