



#ifndef __ONE_BUTTON_DYNAMIC_FILTER_H__
#define __ONE_BUTTON_DYNAMIC_FILTER_H__

#include "ButtonMultiPress.h"

/// \ingroup InputFilter
/// @{
namespace Dasher {
class COneButtonDynamicFilter : public CButtonMultiPress {
 public:
  COneButtonDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate);
  virtual ~COneButtonDynamicFilter();

  virtual bool DecorateView(CDasherView *pView, CDasherInput *pInput);

  virtual bool GetSettings(SModuleSettings **pSettings, int *iCount);

  //override to get mouse clicks / taps back again...
  virtual void KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel);
  virtual void KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel);

 private:
  unsigned int maxClickCount() {return 2;} //double-click to reverse
  virtual void TimerImpl(unsigned long Time, CDasherView *pView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol);
  virtual void ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel* pModel);
  
  int m_iTarget;

  int m_iTargetX[2];
  int m_iTargetY[2];
};
}
/// @}

#endif
