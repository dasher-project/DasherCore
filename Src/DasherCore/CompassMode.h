
// DasherButtons.h
// Copyright 2005 by Chris Ball

#pragma once

#include "DasherButtons.h"

namespace Dasher {
/// \ingroup Input
/// @{
class CCompassMode : public CDasherButtons
{
 public:
  CCompassMode(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface);

  virtual void HandleEvent(Parameter parameter);

  bool DecorateView(CDasherView *pView, CDasherInput *pInput);

  bool GetSettings(SModuleSettings **pSettings, int *iCount);

 protected:
  void SetupBoxes();

 private:
  int iTargetWidth;
};
}
/// @}

