// DynamicFilter.h



#ifndef __ButtonMultiPress_h__
#define __ButtonMultiPress_h__

#include "DynamicButtons.h"

namespace Dasher {
/// \ingroup InputFilter
/// @{
/// DynamicButtons filter which detects long and multiple presses - the latter of the
/// same button, up to maxClickCount() consecutive presses,
/// with a gap of up to LP_MULTIPRESS_TIME ms between the start of _each_pair_ of 
/// presses. Long- and multi-presses are passed onto the standard ButtonEvent method,
/// with iType 1 or to the number of presses, respectively, for subclasses to decide how to respond.
class CButtonMultiPress : public CDynamicButtons {
 public:
  CButtonMultiPress(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName);

  void Timer(unsigned long iTime, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel, CExpansionPolicy **pol);
  void KeyDown(unsigned long iTime, Keys::VirtualKey Key, CDasherView *pView, CDasherInput *pInput, CDasherModel *pModel);

  void pause();
 protected:
  virtual unsigned int maxClickCount()=0;
  void reverse(unsigned long iTime);
  void run(unsigned long iTime);

 private:
  virtual void RevertPresses(int iCount) {};

  Keys::VirtualKey m_iQueueId;
  std::deque<unsigned long> m_deQueueTimes;

  ///Whether a long-press has been handled (in Timer) - as the key
  /// may still be down (and the press becoming ever-longer)!
  bool m_bKeyHandled;
  unsigned long m_iKeyDownTime;
 };
}

#endif
