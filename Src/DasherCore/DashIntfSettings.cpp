#include "DashIntfSettings.h"

using namespace Dasher;

CDashIntfSettings::CDashIntfSettings(CSettingsStore *pSettingsStore)
	: CDasherInterfaceBase(pSettingsStore) {
}

bool CDashIntfSettings::GetBoolParameter(Parameter iParameter) const {
  return m_pSettingsStore->GetBoolParameter(iParameter);
}

long CDashIntfSettings::GetLongParameter(Parameter iParameter) const {
  return m_pSettingsStore->GetLongParameter(iParameter);
}

const std::string &CDashIntfSettings::GetStringParameter(Parameter iParameter) const {
  return m_pSettingsStore->GetStringParameter(iParameter);
}

void CDashIntfSettings::SetBoolParameter(Parameter iParameter, bool bValue) {
  m_pSettingsStore->SetBoolParameter(iParameter, bValue);
}

void CDashIntfSettings::SetLongParameter(Parameter iParameter, long lValue) {
  m_pSettingsStore->SetLongParameter(iParameter, lValue);
}

void CDashIntfSettings::SetStringParameter(Parameter iParameter, const std::string &strValue) {
  m_pSettingsStore->SetStringParameter(iParameter, strValue);
}

bool CDashIntfSettings::IsParameterSaved(std::string & Key) {
    return m_pSettingsStore->IsParameterSaved(Key);
}
