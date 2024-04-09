#ifndef __ModuleSettings_h__
#define __ModuleSettings_h__
#include "Parameters.h"

enum setting_t {
  T_BOOL,
  T_LONG,
  T_LONGSPIN,
  T_STRING
};

typedef struct _SModuleSettings SModuleSettings;

struct _SModuleSettings {
  Dasher::Parameter iParameter;
  setting_t iType;
  int iMin;
  int iMax;
  int iDivisor;
  int iStep;
  const char *szDescription;
};

#endif
