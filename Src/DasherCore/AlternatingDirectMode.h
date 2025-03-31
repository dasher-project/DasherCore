
// DasherButtons.h
// Copyright 2005 by Chris Ball

#pragma once

#include "DasherButtons.h"

namespace Dasher {
/// \ingroup Input
/// @{

//TODO maybe some kind of scanning/menu option here, too, tho slightly more complicated than for direct/menu mode?

  class CAlternatingDirectMode : public CDasherButtons
{
 public:
  CAlternatingDirectMode(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface);

  bool DecorateView(CDasherView *pView, CDasherInput *pInput);

  bool GetSettings(SModuleSettings **pSettings, int *iCount);

 protected:
  void SetupBoxes();

 private:
  void DirectKeyDown(unsigned long iTime, int iId, CDasherView *pView, CDasherModel *pModel);

  int m_iLastBox;
};
}
/// @}

