#include "WordGeneratorBase.h"
#include <sstream>

using namespace Dasher;

CWordGeneratorBase::CWordGeneratorBase(const CAlphInfo *pAlph, const CAlphabetMap *pAlphMap) : m_pAlph(pAlph), m_pAlphMap(pAlphMap) {
}

void CWordGeneratorBase::GetSymbols(std::vector<symbol> &into) {
    into.clear();
    while(into.empty()) {
        std::string s(GetLine());
        if (s.empty()) break; //no more lines, so no more symbols...
        std::stringstream line(s);
        CAlphabetMap::SymbolStream ss(line);

        for (symbol sym = sym=ss.next(m_pAlphMap); sym != -1; sym=ss.next(m_pAlphMap)) {
            if(sym != 0 && m_pAlph->SymbolIsSpaceCharacter(into.back()))
            {
                into.push_back(sym);
            }
        }
        //didn't find any usable symbols in line! repeat...
    }
}

