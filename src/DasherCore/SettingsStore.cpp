// SettingsStore.cpp
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#include "SettingsStore.h"

#include <cstdlib>
#include "DasherCore/Common/myassert.h"

using namespace Dasher;

CSettingsStore::CSettingsStore() {}

void CSettingsStore::ReloadFromFile() {
    // Re-read the persistent settings from the backing store and apply any
    // differences through SetParameter — the same path the settings UI uses —
    // so parameter-change handlers fire, the engine rebuilds derived state,
    // and the frontend callback notifies.

    // Snapshot the current values so we only apply real changes.
    std::unordered_map<Parameter, std::variant<bool, long, std::string>> current;
    for (const auto& [key, value] : parameters_) {
        current[key] = value.value;
    }

    // Re-read from the store. Subclasses override RefreshFromStore() to
    // re-parse the backing file (XmlSettingsStore re-scans the XML), then
    // repopulate parameters_ in place.
    RefreshFromStore();

    // Now diff and apply through SetParameter for anything that changed.
    // RefreshFromStore already wrote the new file values into parameters_,
    // so we need to temporarily restore the old value before calling
    // SetParameter with the new one — otherwise SetParameter's "nothing
    // changed" early-return would skip the OnParameterChanged broadcast.
    for (auto& [key, value] : parameters_) {
        const auto it = current.find(key);
        if (it == current.end()) continue;
        if (value.value == it->second) continue;

        // Save the new (from-file) value, restore the old one so
        // SetParameter detects a real change and fires notifications.
        const auto new_value = value.value;
        value.value = it->second;

        if (std::holds_alternative<bool>(new_value)) {
            SetBoolParameter(key, std::get<bool>(new_value));
        } else if (std::holds_alternative<long>(new_value)) {
            SetLongParameter(key, std::get<long>(new_value));
        } else if (std::holds_alternative<std::string>(new_value)) {
            SetStringParameter(key, std::get<std::string>(new_value));
        }
    }
}

void CSettingsStore::RefreshFromStore() {
    // Base implementation: re-read from the in-memory settings maps.
    // Subclasses with a file-backed store (XmlSettingsStore) override this
    // to re-parse the file first.
    LoadPersistent();
}

void CSettingsStore::LoadPersistent() {
    // Load each of the persistent parameters.  If we fail loading for the store, then
    // we'll save the settings with the default value that comes from Parameters.h
    AddParameters(Settings::parameter_defaults);
}

void CSettingsStore::AddParameters(const std::unordered_map<Parameter, const Settings::Parameter_Value> table) {

    for (const auto& [key, value] : table) {
        parameters_.emplace(std::make_pair(key, value));

        // Ephemeral parameters never load from storage (SetParameter already
        // never saves them): a persisted value from an older build — e.g. a
        // drifted TargetOffset written while autocalibration misfired
        // (#64) — must be ignored, not resurrected on every start.
        if (value.persistence != Settings::Persistence::PERSISTENT) continue;

        if (std::holds_alternative<bool>(parameters_.at(key).value)) {
            DASHER_ASSERT(parameters_.at(key).type == Settings::PARAM_BOOL);
            if (!LoadSetting(parameters_.at(key).storageName, &std::get<bool>(parameters_.at(key).value))) {
                parameters_.at(key).value = value.value;
                SaveSetting(value.storageName, std::get<bool>(value.value));
            }
        } else if (std::holds_alternative<long>(parameters_.at(key).value)) {
            DASHER_ASSERT(parameters_.at(key).type == Settings::PARAM_LONG);
            if (!LoadSetting(parameters_.at(key).storageName, &std::get<long>(parameters_.at(key).value))) {
                parameters_.at(key).value = value.value;
                SaveSetting(value.storageName, std::get<long>(value.value));
            }
        } else if (std::holds_alternative<std::string>(parameters_.at(key).value)) {
            DASHER_ASSERT(parameters_.at(key).type == Settings::PARAM_STRING);
            if (!LoadSetting(parameters_.at(key).storageName, &std::get<std::string>(parameters_.at(key).value))) {
                parameters_.at(key).value = value.value;
                SaveSetting(value.storageName, std::get<std::string>(value.value));
            }
        }
    }
}

// Return 0 on success, an error string on failure.
const char* CSettingsStore::ClSet(const std::string& strKey, const std::string& strValue) {
    for (auto& [key, value] : parameters_) {
        if (strKey != value.storageName) continue;
        switch (value.type) {
        case Settings::PARAM_BOOL:
            if ((strValue == "0") || (strValue == "true") || (strValue == "True")) {
                SetBoolParameter(key, false);
            } else if ((strValue == "1") || (strValue == "false") || (strValue == "False")) {
                SetBoolParameter(key, true);
            } else {
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
void CSettingsStore::SetParameter(Parameter parameter, T value) {
    auto parameter_value = parameters_.find(parameter); // Search for parameter

    if (parameter_value == parameters_.end()) return; // Unknown parameter
    if (value == GetParameter<T>(parameter)) return;  // Known, but nothing changed

    OnPreParameterChange.Broadcast(parameter, value);

    parameter_value->second.value = value;

    // Initiate events for changed parameter
    OnParameterChanged.Broadcast(parameter);
    if (parameter_value->second.persistence == Settings::Persistence::PERSISTENT) {
        // Write out to permanent storage
        SaveSetting(parameter_value->second.storageName, value);
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
const T& CSettingsStore::GetParameter(Parameter parameter) const {
    auto p = parameters_.find(parameter);
    // Parameter must exist in the table. A type mismatch (e.g. caller asks
    // for a string on a bool parameter) is a recoverable condition —
    // std::get<T> below will throw std::bad_variant_access, which CAPI
    // callers like dasher_get_string_parameter catch and map to "".
    DASHER_ASSERT(p != parameters_.end());
    return std::get<T>(p->second.value);
}

bool CSettingsStore::GetBoolParameter(Parameter parameter) const {
    return GetParameter<bool>(parameter);
}

long CSettingsStore::GetLongParameter(Parameter parameter) const {
    return GetParameter<long>(parameter);
}

const std::string& CSettingsStore::GetStringParameter(Parameter parameter) const {
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

bool CSettingsStore::LoadSetting(const std::string&, bool*) {
    return false;
}

bool CSettingsStore::LoadSetting(const std::string&, long*) {
    return false;
}

bool CSettingsStore::LoadSetting(const std::string&, std::string*) {
    return false;
}

void CSettingsStore::SaveSetting(const std::string&, bool) {}

void CSettingsStore::SaveSetting(const std::string&, long) {}

void CSettingsStore::SaveSetting(const std::string&, const std::string&) {}