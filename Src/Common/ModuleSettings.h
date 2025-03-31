#pragma once

enum setting_t {
  T_BOOL,
  T_LONG,
  T_LONGSPIN,
  T_STRING
};

typedef struct _SModuleSettings SModuleSettings;

struct _SModuleSettings {
  int iParameter;
  setting_t iType;
  int iMin;
  int iMax;
  int iDivisor;
  int iStep;
  const char *szDescription;
};

