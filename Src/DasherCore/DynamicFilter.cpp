// DynamicFilter.cpp



#include "DasherInterfaceBase.h"
#include "DynamicFilter.h"
#include <algorithm>

using namespace Dasher;

CDynamicFilter::CDynamicFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CFrameRate *pFramerate, const char *szName)
: CInputFilter(pInterface, szName), m_pFramerate(pFramerate),
  m_pSettingsStore(pSettingsStore), m_bPaused(true) {
}

bool CDynamicFilter::OneStepTowards(CDasherModel *pModel, myint X, myint Y, unsigned long iTime, double dSpeedMul) {
  if (dSpeedMul<=0.0) return false; //going nowhere
  m_pFramerate->RecordFrame(iTime); //Hmmm, even if we don't do anything else?

  // iSteps is the number of update steps we need to get the point
  // under the cursor over to the cross hair. Calculated in order to
  // keep a constant bit-rate.
  const int iSteps(static_cast<int>(m_pFramerate->Steps() / dSpeedMul));
  DASHER_ASSERT(iSteps > 0);
  
  // If X is too large we risk overflow errors, so limit it
  // Not rescaling Y in this case: at that X, all Y's are nearly equivalent!
  X = std::max(myint(1),std::min(X, myint(1<<29)/iSteps));
  
  pModel->ScheduleOneStep(Y-X, Y+X, iSteps, m_pSettingsStore->GetLongParameter(LP_X_LIMIT_SPEED), m_pSettingsStore->GetBoolParameter(BP_EXACT_DYNAMICS));
  return true;
}

double CDynamicFilter::FrameSpeedMul(CDasherModel *pModel, unsigned long iTime) {
  CDasherNode *n = pModel->Get_node_under_crosshair();
  double d = n ? n->SpeedMul() : 1.0;
  if(m_pSettingsStore->GetBoolParameter(BP_SLOW_START)) {
    if ((iTime - m_iStartTime) < static_cast<unsigned long>(m_pSettingsStore->GetLongParameter(LP_SLOW_START_TIME)))
      d *= 0.1 * (1 + 9 * ((iTime - m_iStartTime) / static_cast<double>(m_pSettingsStore->GetLongParameter(LP_SLOW_START_TIME))));
  }
  //else, no slow start, or finished.
  return d;
}

void CDynamicFilter::run(unsigned long Time) {
  if (!isPaused()) return; //already running, no need to / can't really do anything
  
  m_bPaused = false;

  m_pFramerate->Reset_framerate(Time);
  m_iStartTime = Time;
}
