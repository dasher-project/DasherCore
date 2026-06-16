#include "WordGeneratorBase.h"
#include <sstream>

using namespace Dasher;

CWordGeneratorBase::CWordGeneratorBase(const CAlphInfo* pAlph, const CAlphabetMap* pAlphMap)
    : m_pAlph(pAlph), m_pAlphMap(pAlphMap) {}

void CWordGeneratorBase::GetSymbols(std::vector<symbol>& into) {
    into.clear();
    for (;;) {
        std::string s(GetLine());
        if (s.empty()) break;
        std::stringstream line(s);
        CAlphabetMap::SymbolStream ss(line);
        for (symbol sym; (sym = ss.next(m_pAlphMap)) != -1;) {
            if (!into.empty() && sym == 0 && m_pAlph->SymbolIsSpaceCharacter(into.back())) continue;
            if (sym != 0 || !into.empty()) into.push_back(sym);
        }
        if (!into.empty()) {
            if (into.back() == 0) into.pop_back();
            if (!into.empty()) break;
        }
    }
}
