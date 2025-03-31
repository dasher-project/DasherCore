// DashIntfSettings.h
//
// Created 2011 by Alan Lawrence

#ifndef __DASH_INTF_SETTINGS_H__
#define __DASH_INTF_SETTINGS_H__

#include "DasherInterfaceBase.h"

namespace Dasher {
///Class exists only to redefine the settings accessor methods in CDasherInterfaceBase
/// and make them public (otherwise they are protected). As opposed to making them public
/// in CDasherInterfaceBase directly, this has the advantage that it forces all settings
/// accesses to use the proper channel of CSettingsUser _within_DasherCore_ (as all types etc.
/// are CDasherInterfaceBase, and should be kept that way!). However, in platform-specific code,
/// we are often writing stuff in other languages besides C++ and thus cannot get through its
/// access control; hence, rather than each platform-specific subclass of CDasherInterfaceBase
/// redeclaring all the settings methods itself, we'll do it once here.
/// Possible TODO: could make a subclass of CSettingsUser rather than CDasherInterfaceBase, and use multiple inheritance?
class CDashIntfSettings : public CDasherInterfaceBase {
public:

	CDashIntfSettings(CSettingsStore *pSettingsStore);

  bool GetBoolParameter(Parameter iParameter) const;
  long GetLongParameter(Parameter iParameter) const;
  const std::string &GetStringParameter(Parameter iParameter) const;
  void SetBoolParameter(Parameter iParameter, bool bValue);
  void SetLongParameter(Parameter iParameter, long lValue);
  void SetStringParameter(Parameter iParameter, const std::string& strValue);
  bool IsParameterSaved(std::string & Key);
};

}
#endif
