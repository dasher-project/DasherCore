
// 


#pragma once

#include "../Common/ModuleSettings.h"
#include "Parameters.h"
#include <algorithm>
#include <vector>

class CDasherModule;

/// \ingroup Core
/// @{
class CDasherModule {
public:
    CDasherModule(const char *szName);
    virtual ~CDasherModule() = default;

    virtual const char *GetName();

    virtual bool GetSettings(SModuleSettings **pSettings, int *iCount) {
        return false;
    }

private:
    const char *m_szName;

public:
    // Fill this list using the provided method below or via adding to the vector yourself to define which settings should be changable in the UI for the respective module
    virtual void GetUISettings(std::vector<Dasher::Parameter>& List){};
    static void AddSettings(std::vector<Dasher::Parameter>& List, std::vector<Dasher::Parameter> NewParameters){List.insert(List.end(), NewParameters.begin(), NewParameters.end());}
    // Can be used if a specific setting is added by the parent class but overridden or not used by the childclass 
    static void RemoveDeclaredSetting(std::vector<Dasher::Parameter>& List, Dasher::Parameter Param){
        List.erase(std::remove(List.begin(), List.end(), Param), List.end());
    }
};
/// @}

