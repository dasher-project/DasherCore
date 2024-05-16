#pragma once

#include "DasherModule.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Dasher {
  class CDasherInput;
  class CInputFilter;

/// \ingroup Core
/// \{
class CModuleManager {
 public:
    ~CModuleManager();

    CDasherInput* RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault = false);
    CInputFilter* RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault = false);

    CDasherInput* GetDefaultInputDevice();
    void SetDefaultInputDevice(CDasherInput *);
    void ListInputDeviceModules(std::vector<std::string> &vList);
    CDasherInput* GetInputDeviceByName(const std::string strName);

    CInputFilter* GetDefaultInputMethod();
    void SetDefaultInputMethod(CInputFilter *);
    void ListInputMethodModules(std::vector<std::string> &vList);
    CInputFilter* GetInputMethodByName(const std::string strName);

 private:
    std::unordered_map<std::string, CDasherInput*> m_InputDeviceModules;
    std::unordered_map<std::string, CInputFilter*> m_InputMethodModules;

    std::string m_sDefaultInputDevice;
    std::string m_sDefaultInputMethod;
};
/// \}

}
