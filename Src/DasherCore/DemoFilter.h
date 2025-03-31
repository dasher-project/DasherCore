#pragma once

#include "DynamicFilter.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
class CDemoFilter : public CDynamicFilter {
 public:
  CDemoFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate);
  virtual ~CDemoFilter();

  virtual void HandleEvent(Parameter parameter);

  virtual bool DecorateView(CDasherView *pView, CDasherInput *pInput);
  virtual void Timer(unsigned long Time, CDasherView *m_pDasherView, CDasherInput *pInput, CDasherModel *m_pDasherModel, CExpansionPolicy **pol);
  virtual void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel);
  virtual void Activate();
  virtual void Deactivate();
 private:
  double m_dSpring, m_dNoiseNew, m_dNoiseOld;
  double m_dNoiseX, m_dNoiseY;
  myint m_iDemoX, m_iDemoY;
};
}
/// @}

