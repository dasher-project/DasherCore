#pragma once

#include "DasherModule.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "ActionModule.h"


namespace Dasher {
    class Action;
    class CDasherInput;
    class CInputFilter;

/// \ingroup Core
/// \{
class CModuleManager {
 public:
    ~CModuleManager();

    CDasherInput* RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault = false);
    CInputFilter* RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault = false);
    CActionModule* RegisterActionModule(CActionModule* pModule);

    CDasherInput* GetDefaultInputDevice();
    void SetDefaultInputDevice(CDasherInput *);
    void ListInputDeviceModules(std::vector<std::string> &vList);
    CDasherInput* GetInputDeviceByName(const std::string strName);

    CInputFilter* GetDefaultInputMethod();
    void SetDefaultInputMethod(CInputFilter *);
    void ListInputMethodModules(std::vector<std::string> &vList);
    CInputFilter* GetInputMethodByName(const std::string strName);

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Action, T>, void> BroadcastAction(T* Action)
    {
        for(auto& [name, module] : m_ActionModules)
        {
            module->HandleAction(Action);
        }
    };
    
    void ListActionModules(std::vector<std::string> &vList);
    CActionModule* GetActionModuleByName(const std::string strName);

 private:
    std::unordered_map<std::string, CDasherInput*> m_InputDeviceModules;
    std::unordered_map<std::string, CInputFilter*> m_InputMethodModules;
    std::unordered_map<std::string, CActionModule*> m_ActionModules;

    std::string m_sDefaultInputDevice;
    std::string m_sDefaultInputMethod;
};

/// \}

}
