// TwoButtonDynamicFilter.h



#ifndef __TWO_BUTTON_DYNAMIC_FILTER_H__
#define __TWO_BUTTON_DYNAMIC_FILTER_H__

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

#endif
