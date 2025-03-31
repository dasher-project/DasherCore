#include "ModuleManager.h"

#include "DasherInput.h"
#include "InputFilter.h"

using namespace Dasher;

CModuleManager::~CModuleManager() {}

CDasherInput* CModuleManager::RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault)
{
    m_InputDeviceModules[pModule->GetName()] = pModule;
    if(makeDefault) m_sDefaultInputDevice = pModule->GetName();
    return pModule;
}

CInputFilter* CModuleManager::RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault)
{
    m_InputMethodModules[pModule->GetName()] = pModule;
    if(makeDefault) m_sDefaultInputMethod = pModule->GetName();
    return pModule;
}

void CModuleManager::ListInputDeviceModules(std::vector<std::string>& vList)
{
    for(auto& [key,_] : m_InputDeviceModules)
    {
        vList.push_back(key);
    }
}

void CModuleManager::ListInputMethodModules(std::vector<std::string>& vList)
{
    for(auto& [key,_] : m_InputMethodModules)
    {
        vList.push_back(key);
    }
}

CDasherInput* CModuleManager::GetInputDeviceByName(const std::string strName)
{
    if (m_InputDeviceModules.find(strName) != m_InputDeviceModules.end()) {
        return m_InputDeviceModules[strName];
    }
    return nullptr;
}

CInputFilter* CModuleManager::GetInputMethodByName(const std::string strName)
{
    if (m_InputMethodModules.find(strName) != m_InputMethodModules.end()) {
        return m_InputMethodModules[strName];
    }
    return nullptr;
}

CDasherInput *CModuleManager::GetDefaultInputDevice() {
    return m_InputDeviceModules[m_sDefaultInputDevice];
}

CInputFilter *CModuleManager::GetDefaultInputMethod() {
    return m_InputMethodModules[m_sDefaultInputMethod];
}

void CModuleManager::SetDefaultInputDevice(CDasherInput *p) {
  m_sDefaultInputDevice = p->GetName();
}

void CModuleManager::SetDefaultInputMethod(CInputFilter *p) {
  m_sDefaultInputMethod = p->GetName();
}