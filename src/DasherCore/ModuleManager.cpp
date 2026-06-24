// ModuleManager.cpp
//
// Copyright (c) 2008 The Dasher Team
//
// All logic lives in the ModuleMap<T> template in the header.
// This file provides only the destructor and thin forwarding methods.

#include "ModuleManager.h"

#include "DasherInput.h"
#include "InputFilter.h"

using namespace Dasher;

CModuleManager::~CModuleManager() {}

// ── Input device forwarding ──────────────────────────────────────────────

void CModuleManager::RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault) {
    m_InputDevices.Register(pModule, makeDefault);
}

void CModuleManager::RegisterInputDeviceModule(std::unique_ptr<CDasherInput> pModule, bool makeDefault) {
    m_InputDevices.Register(std::move(pModule), makeDefault);
}

CDasherInput* CModuleManager::GetDefaultInputDevice() {
    return m_InputDevices.GetDefault();
}

void CModuleManager::SetDefaultInputDevice(CDasherInput* p) {
    m_InputDevices.SetDefault(p);
}

void CModuleManager::ListInputDeviceModules(std::vector<std::string>& vList) {
    m_InputDevices.List(vList);
}

CDasherInput* CModuleManager::GetInputDeviceByName(const std::string strName) {
    return m_InputDevices.GetByName(strName);
}

// ── Input method forwarding ──────────────────────────────────────────────

void CModuleManager::RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault) {
    m_InputMethods.Register(pModule, makeDefault);
}

void CModuleManager::RegisterInputMethodModule(std::unique_ptr<CInputFilter> pModule, bool makeDefault) {
    m_InputMethods.Register(std::move(pModule), makeDefault);
}

CInputFilter* CModuleManager::GetDefaultInputMethod() {
    return m_InputMethods.GetDefault();
}

void CModuleManager::SetDefaultInputMethod(CInputFilter* p) {
    m_InputMethods.SetDefault(p);
}

void CModuleManager::ListInputMethodModules(std::vector<std::string>& vList) {
    m_InputMethods.List(vList);
}

CInputFilter* CModuleManager::GetInputMethodByName(const std::string strName) {
    return m_InputMethods.GetByName(strName);
}
