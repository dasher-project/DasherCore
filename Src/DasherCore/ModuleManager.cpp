// ModuleManager.cpp
//
// Copyright (c) 2008 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software 
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include <iostream>
#include <stdexcept>

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
