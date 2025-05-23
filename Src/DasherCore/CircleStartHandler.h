#pragma once

#include "StartHandler.h"
#include "SettingsStore.h"
#include "DasherScreen.h"

/// \ingroup Start
/// @{
namespace Dasher {
class CCircleStartHandler : public CStartHandler {
public:
  CCircleStartHandler(CDefaultFilter *pCreator);
  ~CCircleStartHandler();
  virtual bool DecorateView(CDasherView *pView);
  virtual void Timer(unsigned long iTime, dasherint iX, dasherint iY, CDasherView *pView);
  void RegisterView(CDasherView* pView);
  void onPause();
  void onRun(unsigned long iTime);
protected:
  ///Time (as unix timestamp) when user entered circle; max() => already acted upon
  long m_iEnterTime;
  ///Whether the user was in the circle in last call to Timer
  bool m_bInCircle;
  ///Radius of circle in screen coordinates (-1 = needs recomputing)
  int m_iScreenRadius;
  CDasherView *m_pView;
  virtual point CircleCenter(CDasherView *pView);
private:
  ///Cached center of screen circle (needs recomputing if radius does)
  point m_screenCircleCenter;
  CSettingsStore* m_pSettingsStore;
};
}
/// @}

