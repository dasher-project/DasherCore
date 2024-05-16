#include "DasherModule.h"

CDasherModule::CDasherModule(const char *szName): m_szName(szName) {}

const char *CDasherModule::GetName() {
  return m_szName;
}
