// ModuleManager.cpp
//



#include "ModuleManager.h"

#include "DasherInput.h"
#include "InputFilter.h"

using namespace Dasher;

CModuleManager::~CModuleManager() {}

void CModuleManager::RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault)
{
    // Erase possibly known modules with same name
    m_InputDeviceModules.erase(pModule->GetName());
    m_ManagedInputDeviceModules.erase(pModule->GetName());

    if(makeDefault) m_sDefaultInputDevice = pModule->GetName();
    m_InputDeviceModules.emplace(pModule->GetName(), pModule);
}

void CModuleManager::RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault)
{
    // Erase possibly known modules with same name
    m_InputMethodModules.erase(pModule->GetName());
    m_ManagedInputMethodModules.erase(pModule->GetName());

    if(makeDefault) m_sDefaultInputMethod = pModule->GetName();
    m_InputMethodModules.emplace(pModule->GetName(), pModule);
}

void CModuleManager::RegisterInputDeviceModule(std::unique_ptr<CDasherInput> pModule, bool makeDefault)
{
    // Erase possibly known modules with same name
    m_InputDeviceModules.erase(pModule->GetName());
    m_ManagedInputDeviceModules.erase(pModule->GetName());

    if(makeDefault) m_sDefaultInputDevice = pModule->GetName();
    m_ManagedInputDeviceModules.emplace(pModule->GetName(), std::move(pModule));
}

void CModuleManager::RegisterInputMethodModule(std::unique_ptr<CInputFilter> pModule, bool makeDefault)
{
    // Erase possibly known modules with same name
    m_InputMethodModules.erase(pModule->GetName());
    m_ManagedInputMethodModules.erase(pModule->GetName());

    if(makeDefault) m_sDefaultInputMethod = pModule->GetName();
    m_ManagedInputMethodModules.emplace(pModule->GetName(), std::move(pModule));
}

void CModuleManager::ListInputDeviceModules(std::vector<std::string>& vList)
{
    for(auto& [key,_] : m_InputDeviceModules)
    {
        vList.push_back(key);
    }
    for(auto& [key,_] : m_ManagedInputDeviceModules)
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
    for(auto& [key,_] : m_ManagedInputMethodModules)
    {
        vList.push_back(key);
    }
}

CDasherInput* CModuleManager::GetInputDeviceByName(const std::string strName)
{
    if (m_InputDeviceModules.find(strName) != m_InputDeviceModules.end()) {
        return m_InputDeviceModules[strName];
    }
    if (m_ManagedInputDeviceModules.find(strName) != m_ManagedInputDeviceModules.end()) {
        return m_ManagedInputDeviceModules[strName].get();
    }
    return nullptr;
}

CInputFilter* CModuleManager::GetInputMethodByName(const std::string strName)
{
    if (m_InputMethodModules.find(strName) != m_InputMethodModules.end()) {
        return m_InputMethodModules[strName];
    }
    if (m_ManagedInputMethodModules.find(strName) != m_ManagedInputMethodModules.end()) {
        return m_ManagedInputMethodModules[strName].get();
    }
    return nullptr;
}

CDasherInput *CModuleManager::GetDefaultInputDevice() {
    return GetInputDeviceByName(m_sDefaultInputDevice);
}

CInputFilter *CModuleManager::GetDefaultInputMethod() {
    return GetInputMethodByName(m_sDefaultInputMethod);
}

void CModuleManager::SetDefaultInputDevice(CDasherInput *p) {
  m_sDefaultInputDevice = p->GetName();
}

void CModuleManager::SetDefaultInputMethod(CInputFilter *p) {
  m_sDefaultInputMethod = p->GetName();
}