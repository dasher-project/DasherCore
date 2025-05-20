#pragma once

#include "DynamicFilter.h"
#include "AutoSpeedControl.h"
#include "ModuleSettings.h"
#include "StartHandler.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
class CDefaultFilter : public CDynamicFilter {
 public:
  CDefaultFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName);
  ~CDefaultFilter() override;

  /// Responds to changes in BP_START_MOUSE / BP_MOUSEPOS_MODE to create StartHandler
  virtual void HandleParameterChange(Parameter parameter);

  bool DecorateView(CDasherView *pView, CDasherInput *pInput) override;
  void Timer(unsigned long Time, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol) override;
  void ExecuteMovement(unsigned long Time, CDasherView* pView, CDasherModel* pModel, myint newX, myint newY);
  void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel) override;
  void KeyUp(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel) override;
  void Activate() override;
  void Deactivate() override;
  bool GetSettings(SModuleSettings **, int *) override;

  void pause() override;
  //pauses, and calls the interface's Done() method
  void stop();
 protected:
  void CreateStartHandler();
  void run(unsigned long iTime) override;
  virtual CStartHandler *MakeStartHandler();
  virtual void ApplyTransform(myint &iDasherX, myint &iDasherY, CDasherView *pView);
  void ApplyOffset(myint &iDasherX, myint &iDasherY);
  
  /// Last-known Dasher-coords of the target
  myint m_iLastX, m_iLastY;
  bool m_bGotMouseCoords;
private:
  friend class CCircleStartHandler;
  friend class CTwoBoxStartHandler;
  CAutoSpeedControl *m_pAutoSpeedControl;
  myint m_iSum;
  CStartHandler *m_pStartHandler;
  int m_iCounter;
protected:
  bool m_bTurbo;
};
}
/// @}

