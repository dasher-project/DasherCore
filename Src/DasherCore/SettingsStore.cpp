// SettingsStore.cpp
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#include "../Common/Common.h"

#include "SettingsStore.h"
#include "Observable.h"

#include <cstdlib>
#include <iostream>

using namespace Dasher;

CSettingsStore::CSettingsStore() {
}

void CSettingsStore::LoadPersistent() {
	// Load each of the persistent parameters.  If we fail loading for the store, then 
	// we'll save the settings with the default value that comes from Parameters.h
	AddParameters(Settings::parameter_defaults);
}

void CSettingsStore::AddParameters(const std::unordered_map<Parameter, const Settings::Parameter_Value> table) {

	for(auto [key, value] : table)
	{
		parameters_[key] = value;

		if(std::holds_alternative<bool>(parameters_[key].value))
		{
			DASHER_ASSERT(parameters_[key].type == Settings::ParamBool);
			if (!LoadSetting(parameters_[key].name, &std::get<bool>(parameters_[key].value))) {
		      parameters_[key].value = value.value;
		      SaveSetting(value.name, std::get<bool>(value.value));
		    }
		}else
		if(std::holds_alternative<long>(parameters_[key].value))
		{
			DASHER_ASSERT(parameters_[key].type == Settings::ParamLong);
			if (!LoadSetting(parameters_[key].name, &std::get<long>(parameters_[key].value))) {
		      parameters_[key].value = value.value;
		      SaveSetting(value.name, std::get<long>(value.value));
		    }
		}else
		if(std::holds_alternative<std::string>(parameters_[key].value))
		{
			DASHER_ASSERT(parameters_[key].type == Settings::ParamString);
			if (!LoadSetting(parameters_[key].name, &std::get<std::string>(parameters_[key].value))) {
		      parameters_[key].value = value.value;
		      SaveSetting(value.name, std::get<std::string>(value.value));
		    }
		}
	}
}

// Return 0 on success, an error string on failure.
const char * CSettingsStore::ClSet(const std::string &strKey, const std::string &strValue) {
	for (auto& [key, value] : parameters_) {
		if(strKey != value.name) continue;
		switch (value.type) {
			case Settings::PARAM_BOOL: 
				if ((strValue == "0") || (strValue == "true") || (strValue == "True")){
					SetBoolParameter(key, false);
                }else if((strValue == "1") || (strValue == "false") || (strValue == "False")){
					SetBoolParameter(key, true);
                }else{
					// Note to translators: This message will be output for a command line
					// with "--options foo=VAL" and foo is a boolean valued parameter, but
					// "VAL" is not true or false.
					return "boolean value must be specified as 'true' or 'false'.";
				}
				return nullptr;
			case Settings::PARAM_LONG: 
				// TODO: check the string to int conversion result.
				SetLongParameter(key, atoi(strValue.c_str()));
				return nullptr;

			case Settings::PARAM_STRING: 
				SetStringParameter(key, strValue);
				return nullptr;

			case Settings::PARAM_INVALID:
                return "Unknown parameter given";
		}
	}
	// Note to translators: This is output when command line "--options" doesn't
	// specify a known option.
	return "unknown option, use \"--help-options\" for more information.";
}


/* TODO: Consider using Template functions to make this neater. */

template <typename T>
void CSettingsStore::SetParameter(Parameter parameter, T value)
{
    auto parameter_value = parameters_.find(parameter); // Search for parameter

    if(parameter_value == parameters_.end()) return; // Unknown parameter
	if(value == GetParameter<T>(parameter)) return; // Known, but nothing changed

	pre_set_observable_.DispatchEvent(CParameterChange(parameter,value));

	parameter_value->second.value = value;

	// Initiate events for changed parameter
	DispatchEvent(parameter);
	if (parameter_value->second.persistence == Settings::Persistence::PERSISTENT) {
		// Write out to permanent storage
		SaveSetting(parameter_value->second.name, value);
	}
}

void CSettingsStore::SetBoolParameter(Parameter parameter, bool bValue) {
	SetParameter(parameter, bValue);
}

void CSettingsStore::SetLongParameter(Parameter parameter, long lValue) {
	SetParameter(parameter, lValue);
}

void CSettingsStore::SetStringParameter(Parameter parameter, const std::string sValue) {
	SetParameter(parameter, sValue);
}

template <typename T>
const T& CSettingsStore::GetParameter(Parameter parameter) const
{
	auto p = parameters_.find(parameter);
	// Check that the parameter is in fact in the right spot in the table
	DASHER_ASSERT(p != parameters_.end() && std::holds_alternative<T>(p->second.type));
	return std::get<T>(p->second.value);
}

bool CSettingsStore::GetBoolParameter(Parameter parameter) const {
  return GetParameter<bool>(parameter);
}

long CSettingsStore::GetLongParameter(Parameter parameter) const {
   return GetParameter<long>(parameter);
}

const std::string &CSettingsStore::GetStringParameter(Parameter parameter) const {
   return GetParameter<std::string>(parameter);
}

void CSettingsStore::ResetParameter(Parameter parameter) {
	auto current_parameter = parameters_.find(parameter);
	auto default_parameter = Settings::parameter_defaults.find(parameter);
	DASHER_ASSERT(current_parameter != parameters_.end() && default_parameter != Settings::parameter_defaults.end());

	current_parameter->second.value = default_parameter->second.value;
}

/* Private functions -- Settings are not saved between sessions unless these
functions are over-ridden.
--------------------------------------------------------------------------*/

bool CSettingsStore::LoadSetting(const std::string &, bool *) {
  return false;
}

bool CSettingsStore::LoadSetting(const std::string &, long *) {
  return false;
}

bool CSettingsStore::LoadSetting(const std::string &, std::string *) {
  return false;
}

void CSettingsStore::SaveSetting(const std::string &, bool ) {
}

void CSettingsStore::SaveSetting(const std::string &, long ) {
}

void CSettingsStore::SaveSetting(const std::string &, const std::string &) {
}

/* SettingsUser and SettingsObserver definitions... */

CSettingsUser::CSettingsUser(CSettingsStore *pSettingsStore) {
	//ATM, we only allow one settings store, total...
	DASHER_ASSERT(s_pSettingsStore==NULL);
	//but in future, remove that if we're allowing multiple settings stores to exist
	// concurrently.
	s_pSettingsStore = pSettingsStore;
}

CSettingsUser::CSettingsUser(CSettingsUser *pCreateFrom) {
	//No need to do anything atm; but in future, copy CSettingsStore pointer
	// from argument.
	DASHER_ASSERT(pCreateFrom);
	s_pSettingsStore = pCreateFrom->GetSettingsStore();
}

CSettingsUser::~CSettingsUser() = default;

bool CSettingsUser::GetBoolParameter(Parameter parameter) const
{
	return s_pSettingsStore->GetBoolParameter(parameter);
}
long CSettingsUser::GetLongParameter(Parameter parameter) const
{
	return s_pSettingsStore->GetLongParameter(parameter);
}
const std::string &CSettingsUser::GetStringParameter(Parameter parameter) const
{
	return s_pSettingsStore->GetStringParameter(parameter);
}
void CSettingsUser::SetBoolParameter(Parameter parameter, bool bValue)
{
	s_pSettingsStore->SetBoolParameter(parameter, bValue);
}
void CSettingsUser::SetLongParameter(Parameter parameter, long lValue)
{
	s_pSettingsStore->SetLongParameter(parameter, lValue);
}
void CSettingsUser::SetStringParameter(Parameter parameter, const std::string &strValue)
{
	s_pSettingsStore->SetStringParameter(parameter, strValue);
}

bool CSettingsUser::IsParameterSaved(const std::string &Key)
{
	return s_pSettingsStore->IsParameterSaved(Key);
}

CSettingsObserver::CSettingsObserver(CSettingsUser *pCreateFrom) {
  DASHER_ASSERT(pCreateFrom);
  s_pSettingsStore = pCreateFrom;
  s_pSettingsStore->GetSettingsStore()->Register(this);
}

CSettingsObserver::~CSettingsObserver() {
  s_pSettingsStore->GetSettingsStore()->Unregister(this);
}

CSettingsUserObserver::CSettingsUserObserver(CSettingsUser *pCreateFrom)
: CSettingsUser(pCreateFrom), CSettingsObserver(pCreateFrom) {
}
