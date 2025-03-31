// TwoButtonDynamicFilter.h
//
// Copyright (c) 2007 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software 
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once

#include "ButtonMultiPress.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
class CTwoButtonDynamicFilter : public CButtonMultiPress {
 public:
  CTwoButtonDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate);
  virtual ~CTwoButtonDynamicFilter();
 

  // Inherited methods
  bool DecorateView(CDasherView *pView, CDasherInput *pInput) override;

  bool GetSettings(SModuleSettings **pSettings, int *iCount) override;

  bool GetMinWidth(int &iMinWidth) override;

  
 protected:
  //override to inspect x,y coords of mouse clicks/taps
  void KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel) override;
  void KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel) override;
	
 private:
  unsigned int maxClickCount() override {return m_pSettingsStore->GetBoolParameter(BP_2B_INVERT_DOUBLE) ? 3 : 2;}
  void TimerImpl(unsigned long Time, CDasherView *m_pDasherView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol) override;
  void ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel* pModel) override;
  virtual void ComputeLagBits();

  double m_dLagBits;
  ///id of physical key, whose pressing we have emulated, in response
  /// to a mouse down event on one or other half of the canvas...
  Keys::VirtualKey m_iMouseButton;
};
}
/// @}

