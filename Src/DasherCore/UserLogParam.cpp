
#include "../Common/Common.h"

#include "UserLogParam.h"



// Needed so we can sort() vectors of parameters
bool CUserLogParam::ComparePtr(CUserLogParam* pA, CUserLogParam* pB)
{
  if ((pA == NULL) || (pB == NULL))
    return false;
  int iResult = pA->strName.compare(pB->strName);

  if (iResult < 0)
    return true;
  else if (iResult == 0)
  {
    if (pA->strTimeStamp.compare(pB->strTimeStamp) < 0)
      return true;
  }

  return false;
}


