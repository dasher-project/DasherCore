#include "StaticFilter.h"
#include "InputFilter.h"

using namespace Dasher;

CStaticFilter::CStaticFilter(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pIntf, const char* szName)
    : CInputFilter(pIntf, szName), m_pSettingsStore(pSettingsStore), m_pModel(NULL) {}

void CStaticFilter::pause() {
    if (m_pModel) m_pModel->ClearScheduledSteps();
}

void CStaticFilter::ScheduleZoom(CDasherModel* pModel, myint y1, myint y2) {
    (m_pModel = pModel)->ScheduleZoom(y1, y2, m_pSettingsStore->GetLongParameter(LP_ZOOMSTEPS));
}

void CStaticFilter::GetUISettings(std::vector<Dasher::Parameter>& List) {
    CInputFilter::GetUISettings(List);
    AddSettings(List, {LP_ZOOMSTEPS});
}