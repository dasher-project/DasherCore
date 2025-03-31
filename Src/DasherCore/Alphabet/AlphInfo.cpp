#include "AlphInfo.h"

using namespace Dasher;

CAlphInfo::CAlphInfo() {
  //Members of SGroupInfo:
    pChild=nullptr;
    pNext=nullptr;
    iStart=1;
    iEnd=1;
    iNumChildNodes = 0;

    m_iConversionID = None;
    m_strConversionTrainStart = "<";
    m_strConversionTrainStop = ">";
    m_strDefaultContext = ". ";
    m_strCtxChar = "§";
}

std::string CAlphInfo::escape(const std::string &ch) const {
    if ((!m_strConversionTrainStart.empty() && ch==m_strConversionTrainStart)
        || (!m_strCtxChar.empty() && ch==m_strCtxChar))
        return ch+ch;
  return ch;
}

CAlphInfo::~CAlphInfo() {
    for(auto action_vector : m_vCharacterDoActions)
    {
        for(auto a : action_vector) delete a;
        action_vector.clear();
    }
    for(auto action_vector : m_vCharacterUndoActions)
    {
        for(auto a : action_vector) delete a;
        action_vector.clear();
    }
}

void CAlphInfo::copyCharacterFrom(const CAlphInfo *other, int idx) {
    m_vCharacters.push_back(other->m_vCharacters[idx-1]);
}
