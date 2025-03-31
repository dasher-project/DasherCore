// DasherModule.h
// 


#ifndef DASHER_MODULE_H
#define DASHER_MODULE_H

#include "../Common/ModuleSettings.h"

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
};
/// @}

#endif
