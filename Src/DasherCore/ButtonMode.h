// ButtonMode.h 
// Copyright 2009 by Alan Lawrence

#ifndef __BUTTON_MODE_H__
#define __BUTTON_MODE_H__

#include "DasherButtons.h"

namespace Dasher {
/// \ingroup Input
/// @{
/// Handles the "menu mode" and "direct mode" input filters, according to the bMenu constructor parameter.
class CButtonMode : public CDasherButtons
{
 public:
  CButtonMode(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, bool bMenu, const char *szName);
  ~CButtonMode() override;

  void Timer(unsigned long Time, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol) override;
  bool DecorateView(CDasherView *pView, CDasherInput *pInput) override;

  //override to get mouse clicks/taps back again
  void KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) override;
  
  bool GetSettings(SModuleSettings **pSettings, int *iCount) override;
 protected: 
  void SetupBoxes() override;
  void DirectKeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView* pView, CDasherModel* pModel) override;
 private:
  bool m_bHighlight;
  unsigned long m_iLastTime;
};
}
/// @}

#endif
