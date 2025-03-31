



#ifndef __TWO_PUSH_DYNAMIC_FILTER_H__
#define __TWO_PUSH_DYNAMIC_FILTER_H__

#include "DynamicButtons.h"
namespace Dasher {
/// \ingroup InputFilter
/// @{
  ///Dynamic filter in which user has two gestures, both performed with a single
  /// button: one gesture for up and one for down. According to BP_TWO_PUSH_RELEASE_TIME,
  /// the two gestures are either (a) two pushes with a short gap between, vs
  /// (b) two pushes with a long gap between; or else, (a) a short vs (b) a long press.
  ///Note we do not detect long or multiple presses in the usual way (CButtonMultiPress):
  /// for users capable of making long presses, we suggest turning BP_TWO_PUSH_RELEASE_TIME
  /// on (and reversing can be achieved by making a press of ambiguous length -
  /// too short, too long, or inbetween); otherwise, with BP_TWO_PRESS_RELEASE_TIME
  /// off, reversing can be achieved by making just a single press, and then waiting.
  class CTwoPushDynamicFilter : public CDynamicButtons {
 public:
  CTwoPushDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate);
  virtual ~CTwoPushDynamicFilter();
  
  // Inherited methods
  virtual bool DecorateView(CDasherView *pView, CDasherInput *pInput);
 
  virtual bool GetMinWidth(int &iMinWidth);
  virtual bool GetSettings(SModuleSettings **pSettings, int *iCount);

  //override to get mouse clicks / taps back again if BACKOFF_BUTTON off...
  virtual void KeyDown(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel);
  virtual void KeyUp(unsigned long Time, Keys::VirtualKey Key, CDasherView *pDasherView, CDasherInput *pInput, CDasherModel *pModel);

 protected:
  virtual void TimerImpl(unsigned long Time, CDasherView *m_pDasherView, CDasherModel *m_pDasherModel, CExpansionPolicy **pol);
  virtual void ActionButton(unsigned long iTime, Keys::VirtualKey Key, int iType, CDasherModel* pModel);

  virtual void HandleParameterChange(Parameter parameter);

  virtual void run(unsigned long iTime);

 private:
  void updateBitrate(double dBitrate);
  long upDist();
  long downDist();
  double m_dLogUpMul, m_dLogDownMul, m_dLagBits;
  double m_dMinShortTwoPushTime, m_dMaxShortTwoPushTime,
    m_dMinLongTwoPushTime, m_dMaxLongTwoPushTime;
  int m_aiMarker[2];
  int m_iActiveMarker;
  int m_aiTarget[2];
  int m_aaiGuideAreas[2][2];
  double m_dLastBitRate;
  double m_dNatsSinceFirstPush;
};
}
/// @}

#endif
