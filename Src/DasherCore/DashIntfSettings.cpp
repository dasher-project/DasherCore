#include "DashIntfSettings.h"

using namespace Dasher;
using std::string;

CDashIntfSettings::CDashIntfSettings(CSettingsStore *pSettingsStore)
	: CDasherInterfaceBase(pSettingsStore) {
}

bool CDashIntfSettings::GetBoolParameter(Parameter iParameter) const {
  return CDasherInterfaceBase::GetBoolParameter(iParameter);
}

long CDashIntfSettings::GetLongParameter(Parameter iParameter) const {
  return CDasherInterfaceBase::GetLongParameter(iParameter);
}

const string &CDashIntfSettings::GetStringParameter(Parameter iParameter) const {
  return CDasherInterfaceBase::GetStringParameter(iParameter);
}

void CDashIntfSettings::SetBoolParameter(Parameter iParameter, bool bValue) {
  CDasherInterfaceBase::SetBoolParameter(iParameter, bValue);
}

void CDashIntfSettings::SetLongParameter(Parameter iParameter, long lValue) {
  CDasherInterfaceBase::SetLongParameter(iParameter, lValue);
}

void CDashIntfSettings::SetStringParameter(Parameter iParameter, const string &strValue) {
  CDasherInterfaceBase::SetStringParameter(iParameter, strValue);
}

bool CDashIntfSettings::IsParameterSaved(std::string & Key) {
    return CDasherInterfaceBase::IsParameterSaved(Key);
}
