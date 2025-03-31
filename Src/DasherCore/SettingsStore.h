// SettingsStore.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include "Event.h"
#include "Parameters.h"

namespace Dasher {
/// \ingroup Core
/// @{

/// \brief Abstract representation of persistant storage.
///
/// Stores current runtime _values_ of all BP_, LP_, and SP_ preferences;
/// subclasses may load these from and persist them to disk;
/// is also an Observable for things that want to be notified when prefs change.
///
/// At present we allow for only one global SettingsStore across the whole of Dasher,
/// but the framework should allow for multiple SettingsStores with only minor changes.
/// (The exact use case for multiple SettingsStore's is not clear, and one suggestion is
/// that they should all share the same runtime data - perhaps persisting to different
/// locations. This requires only (a) defining SettingsObservers which persist changes
/// to arbitrary locations, or (b) make the actual pref-value data static i.e. shared
/// between instances.)
///
/// The public interface uses UTF-8 strings. All Keys should be
/// in American English and encodable in ASCII. However,
/// string Values may contain special characters where appropriate.
class CSettingsStore {
public:

  CSettingsStore();

  virtual ~CSettingsStore() = default;

  // New functions for event driven interface

    template<typename T> void SetParameter(Parameter parameter, T value);
  void SetBoolParameter(Parameter parameter, bool bValue);
  void SetLongParameter(Parameter parameter, long lValue);
  void SetStringParameter(Parameter parameter, const std::string sValue);

    template<typename T> const T& GetParameter(Parameter parameter) const;
  bool GetBoolParameter(Parameter parameter) const;
  long GetLongParameter(Parameter parameter) const;
  const std::string &GetStringParameter(Parameter parameter) const;

  void ResetParameter(Parameter parameter);

  const char *ClSet(const std::string &strKey, const std::string &strValue);

  // TODO: just load the application parameters by default?
  void AddParameters(const std::unordered_map<Parameter, const Settings::Parameter_Value> table);

    Event<Parameter, std::variant<bool, long, std::string>> OnPreParameterChange;
    Event<Parameter> OnParameterChanged;
    
  virtual bool IsParameterSaved(const std::string & Key) { return false; }; // avoid undef sub-classes error

protected:
    ///Loads all (persistent) prefs from disk, using+storing default values when no
    /// existing value stored; non-persistent prefs are reinitialized from defaults.
    void LoadPersistent();
    
private:
  // Platform Specific settings file management

  // LoadSetting changes Value only if it succeeds in loading the setting,
  // in which case it also returns true. Failure is indicated by returning false.
  //! Load a setting with a boolean value
  //
  //! Load a setting with a boolean value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual bool LoadSetting(const std::string & Key, bool * Value);

  //! Load a setting with a long value
  //
  //! Load a setting with a long value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual bool LoadSetting(const std::string & Key, long *Value);

  //! Load a setting with a string value
  //
  //! Load a setting with a string value. Return true if successful
  //! \param Key Name of the setting
  //! \param Value Value of the setting, UTF8 encoded
  virtual bool LoadSetting(const std::string & Key, std::string * Value);

  //! Save a setting with a boolean value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual void SaveSetting(const std::string & Key, bool Value);

  //! Save a setting with a long value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting
  virtual void SaveSetting(const std::string & Key, long Value);

  //! Save a setting with a string value
  //
  //! \param Key Name of the setting
  //! \param Value Value of the setting, UTF8 encoded
  virtual void SaveSetting(const std::string & Key, const std::string & Value);

  std::unordered_map<Parameter, Settings::Parameter_Value> parameters_;
};
}
