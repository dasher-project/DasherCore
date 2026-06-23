// ModuleManager.h
//
// Copyright (c) 2008 The Dasher Team
//
// Manages registration, lookup, and default selection for engine modules
// (input devices and input method filters). Uses a generic ModuleMap<T>
// helper to avoid duplicating the identical bookkeeping logic for each
// module type.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Dasher {
class CDasherInput;
class CInputFilter;

/// Generic module registry for a single module type (e.g. CDasherInput,
/// CInputFilter). Handles both externally-owned modules (raw pointer) and
/// self-owned modules (unique_ptr). Each module is keyed by its name
/// (from T::GetName()). At most one default module can be active at a time.
template <typename T>
class ModuleMap {
  public:
    /// Register an externally-owned module. The caller retains ownership.
    void Register(T* pModule, bool makeDefault = false) {
        const std::string name = pModule->GetName();
        m_External.erase(name);
        m_Managed.erase(name);
        if (makeDefault) m_DefaultName = name;
        m_External.emplace(name, pModule);
    }

    /// Register a self-owned module. Ownership transfers to this ModuleMap.
    void Register(std::unique_ptr<T> pModule, bool makeDefault = false) {
        const std::string name = pModule->GetName();
        m_External.erase(name);
        m_Managed.erase(name);
        if (makeDefault) m_DefaultName = name;
        m_Managed.emplace(name, std::move(pModule));
    }

    /// Collect all registered module names into the output vector.
    void List(std::vector<std::string>& vList) const {
        for (const auto& [key, _] : m_External)
            vList.push_back(key);
        for (const auto& [key, _] : m_Managed)
            vList.push_back(key);
    }

    /// Look up a module by name. Returns nullptr if not found.
    /// Searches both externally-owned and self-owned registrations.
    T* GetByName(const std::string& name) const {
        auto extIt = m_External.find(name);
        if (extIt != m_External.end()) return extIt->second;
        auto mgdIt = m_Managed.find(name);
        if (mgdIt != m_Managed.end()) return mgdIt->second.get();
        return nullptr;
    }

    /// Return the default module, or nullptr if none is set.
    T* GetDefault() const { return GetByName(m_DefaultName); }

    /// Set the default module by pointer.
    void SetDefault(T* p) { m_DefaultName = p->GetName(); }

  private:
    std::unordered_map<std::string, T*> m_External;                ///< externally-owned modules
    std::unordered_map<std::string, std::unique_ptr<T>> m_Managed; ///< self-owned modules
    std::string m_DefaultName;                                     ///< name of the default module (empty = none)
};

/// Manages input device and input method filter registrations.
/// Thin wrapper around two ModuleMap instances — all logic is in the template.
class CModuleManager {
  public:
    ~CModuleManager();

    // ── Input device modules (CDasherInput) ──────────────────────────────
    void RegisterInputDeviceModule(CDasherInput* pModule, bool makeDefault = false);
    void RegisterInputDeviceModule(std::unique_ptr<CDasherInput> pModule, bool makeDefault = false);
    CDasherInput* GetDefaultInputDevice();
    void SetDefaultInputDevice(CDasherInput*);
    void ListInputDeviceModules(std::vector<std::string>& vList);
    CDasherInput* GetInputDeviceByName(const std::string strName);

    // ── Input method modules (CInputFilter) ──────────────────────────────
    void RegisterInputMethodModule(CInputFilter* pModule, bool makeDefault = false);
    void RegisterInputMethodModule(std::unique_ptr<CInputFilter> pModule, bool makeDefault = false);
    CInputFilter* GetDefaultInputMethod();
    void SetDefaultInputMethod(CInputFilter*);
    void ListInputMethodModules(std::vector<std::string>& vList);
    CInputFilter* GetInputMethodByName(const std::string strName);

  private:
    ModuleMap<CDasherInput> m_InputDevices;
    ModuleMap<CInputFilter> m_InputMethods;
};

} // namespace Dasher
