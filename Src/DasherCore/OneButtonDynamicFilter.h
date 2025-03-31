



#pragma once

#include "ButtonMultiPress.h"

/// \ingroup InputFilter
/// @{
namespace Dasher {
class COneButtonDynamicFilter : public CButtonMultiPress {
 public:
  COneButtonDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate);
  virtual ~COneButtonDynamicFilter();

  virtual bool DecorateView(CDasherView *pView, CDasherInput *pInput) override;

  virtual bool GetSettings(SModuleSettings **pSettings, int *iCount) override;

  //override to get mouse clicks / taps back again...
  virtual void KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) override;
  virtual void KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) override;

 private:
  unsigned int maxClickCount() override {return 2;} //double-click to reverse
  virtual void TimerImpl(unsigned long Time, CDasherView *pView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol) override;
  virtual void ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel* pModel) override;
  
  int m_iTarget;

  int m_iTargetX[2];
  int m_iTargetY[2];
};
}
/// @}

