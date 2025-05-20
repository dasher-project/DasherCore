#pragma once

#include "Parameters.h"
#include <string>
#include <unordered_map>

enum setting_t {
  T_BOOL,
  T_LONG,
  T_LONGSPIN,
  T_STRING
};

typedef struct _SModuleSettings SModuleSettings;

struct _SModuleSettings {
  int iParameter;
  setting_t iType;
  int iMin;
  int iMax;
  int iDivisor;
  int iStep;
  const char *szDescription;
};

enum SettingsType
{
  Switch,
  TextField,
  Slider,
  Enum,
  Step
};

namespace Dasher {
  namespace Settings {
    struct ModuleSetting
    {
      SettingsType Type;
      ModuleSetting(SettingsType Type, Dasher::Parameter Param, std::string Description) : Type(Type), Param(Param), Description(Description) {}
      virtual ~ModuleSetting() {};
      
      std::string Description;
      Dasher::Parameter Param;
    };
    
    struct TextSetting : ModuleSetting {
      TextSetting(Dasher::Parameter Param, std::string Description) : ModuleSetting(SettingsType::TextField, Param, Description) {}
    };
    
    struct SliderSetting : ModuleSetting {
      SliderSetting(Dasher::Parameter Param, std::string Description, int min, int max, int step) : ModuleSetting(SettingsType::Slider, Param, Description), min(min), max(max), step(step) {};
      int min;
      int max;
      int step;
    };

    struct SpinSetting : ModuleSetting {
      SpinSetting(Dasher::Parameter Param, std::string Description, int min, int max, int step) : ModuleSetting(SettingsType::Slider, Param, Description), min(min), max(max), step(step) {}
      int min;
      int max;
      int step;
    };

    struct EnumSetting : ModuleSetting {
      EnumSetting(Dasher::Parameter Param, std::string Description, std::unordered_map<std::string, int> Enums) : ModuleSetting(SettingsType::Enum, Param, Description), Enums(Enums) {};
      std::unordered_map<std::string, int> Enums;
    };

    struct SwitchSetting : ModuleSetting {
      SwitchSetting(Dasher::Parameter Param, std::string Description) : ModuleSetting(SettingsType::Switch, Param, Description) {};
    };
  }
}
