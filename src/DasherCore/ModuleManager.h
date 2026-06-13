#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Dasher {
class CDasherInput;
class CInputFilter;

class CModuleManager {
  public:
    ~CModuleManager();

    // Externally Memory Managed Modules
    void RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault = false);
    void RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault = false);
    // Memory Managed Modules
    void RegisterInputDeviceModule(std::unique_ptr<CDasherInput> pModule, bool makeDefault = false);
    void RegisterInputMethodModule(std::unique_ptr<CInputFilter> pModule, bool makeDefault = false);

    CDasherInput* GetDefaultInputDevice();
    void SetDefaultInputDevice(CDasherInput*);
    void ListInputDeviceModules(std::vector<std::string>& vList);
    CDasherInput* GetInputDeviceByName(const std::string strName);

    CInputFilter* GetDefaultInputMethod();
    void SetDefaultInputMethod(CInputFilter*);
    void ListInputMethodModules(std::vector<std::string>& vList);
    CInputFilter* GetInputMethodByName(const std::string strName);

  private:
    std::unordered_map<std::string, CDasherInput*> m_InputDeviceModules;
    std::unordered_map<std::string, CInputFilter*> m_InputMethodModules;
    std::unordered_map<std::string, std::unique_ptr<CDasherInput>> m_ManagedInputDeviceModules;
    std::unordered_map<std::string, std::unique_ptr<CInputFilter>> m_ManagedInputMethodModules;

    std::string m_sDefaultInputDevice;
    std::string m_sDefaultInputMethod;
};

} // namespace Dasher
