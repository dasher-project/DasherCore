// PPMPYLanguageModel.cpp
//
// Mandarin character - py prediction by a extension in PPM (subtrees attached to Symbol nodes)
//
// Started from a replicate of PPMLanguageModel
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2005 David Ward
//
/////////////////////////////////////////////////////////////////////////////

#include "PPMPYLanguageModel.h"
#include "LanguageModel.h"
#include "PPMLanguageModel.h"

#include <algorithm>
#include "DasherCore/DasherTypes.h"
#include "DasherCore/Common/myassert.h"

using namespace Dasher;

/////////////////////////////////////////////////////////////////////

CPPMPYLanguageModel::CPPMPYLanguageModel(CSettingsStore* pSettingsStore, int iNumCHsyms, int iNumPYsyms)
    : CAbstractPPM(pSettingsStore, iNumCHsyms, new CPPMPYnode(-1), 2), NodesAllocated(0), m_NodeAlloc(8192),
      m_iNumPYsyms(iNumPYsyms) {}

/////////////////////////////////////////////////////////////////////
// Get the probability distribution at the context

// ACL This is Will's original GetProbs() method - AFAICT unused, as the only
//  potential call site tested for MandarinDasher and if so explicitly called
//  the old GetPYProbs instead (!)...so have renamed latter to Get Probs instead...
/*void CPPMPYLanguageModel::GetProbs(Context context, std::vector<unsigned int> &probs, int norm, int iUniform) const {
  const CPPMPYContext *ppmcontext = (const CPPMPYContext *)(context);

  //  DASHER_ASSERT(m_setContexts.count(ppmcontext) > 0);


  int iNumSymbols = GetSize();

  probs.resize(iNumSymbols);

  std::vector < bool > exclusions(iNumSymbols);

  unsigned int iToSpend = norm;
  unsigned int iUniformLeft = iUniform;

  // TODO: Sort out zero symbol case
  probs[0] = 0;
  exclusions[0] = false;

  int i;
  for(i = 1; i < iNumSymbols; i++) {
    probs[i] = iUniformLeft / (iNumSymbols - i);
    iUniformLeft -= probs[i];
    iToSpend -= probs[i];
    exclusions[i] = false;
  }

  DASHER_ASSERT(iUniformLeft == 0);

  //  bool doExclusion = GetLongParameter( LP_LM_ALPHA );
  bool doExclusion = 0; //FIXME

  int alpha = GetLongParameter( LP_LM_ALPHA );
  int beta = GetLongParameter( LP_LM_BETA );

  CPPMPYnode *pTemp = ppmcontext->head;

  while(pTemp != 0) {
    int iTotal = 0;

    CPPMPYnode *pSymbol;
    for(i =0; i<DIVISION; i++){
      pSymbol = pTemp->child[i];
      while(pSymbol) {
    int sym = pSymbol->symbol;

    if(!(exclusions[sym] && doExclusion))
      iTotal += pSymbol->count;
    pSymbol = pSymbol->next;
      }
    }

    if(iTotal) {
      unsigned int size_of_slice = iToSpend;

      for(i=0; i<DIVISION; i++){
    pSymbol = pTemp->child[i];
    while(pSymbol) {
      if(!(exclusions[pSymbol->symbol] && doExclusion)) {
        exclusions[pSymbol->symbol] = 1;

        unsigned int p = static_cast < myint > (size_of_slice) * (100 * pSymbol->count - beta) / (100 * iTotal + alpha);

        if((pSymbol->symbol>-1)&&(pSymbol->symbol<GetSize())){
          probs[pSymbol->symbol] += p;
          iToSpend -= p;
        }
      }
      //                              Usprintf(debug,TEXT("sym %u counts %d p %u tospend %u
\n"),sym,s->count,p,tospend);
      //                              DebugOutput(debug);
      pSymbol = pSymbol->next;
    }
      }

    }
    pTemp = pTemp->vine;
  }

  unsigned int size_of_slice = iToSpend;
  int symbolsleft = 0;

  for(i = 1; i < iNumSymbols; i++)
    if(!(exclusions[i] && doExclusion))
      symbolsleft++;

//      std::ostringstream str;
//      for (sym=0;sym<modelchars;sym++)
//              str << probs[sym] << " ";
//      str << std::endl;
//      DASHER_TRACEOUTPUT("probs %s",str.str().c_str());

//      std::ostringstream str2;
//      for (sym=0;sym<modelchars;sym++)
//              str2 << valid[sym] << " ";
//      str2 << std::endl;
//      DASHER_TRACEOUTPUT("valid %s",str2.str().c_str());

  for(i = 1; i < iNumSymbols; i++) {
    if(!(exclusions[i] && doExclusion)) {
      unsigned int p = size_of_slice / symbolsleft;
      probs[i] += p;
      iToSpend -= p;
    }
  }

  int iLeft = iNumSymbols-1;

  for(int j = 1; j < iNumSymbols; ++j) {
    unsigned int p = iToSpend / iLeft;
    probs[j] += p;
    --iLeft;
    iToSpend -= p;
  }

  DASHER_ASSERT(iToSpend == 0);
}*/

void CPPMPYLanguageModel::GetPartProbs(Context context, std::vector<std::pair<symbol, unsigned int>>& vChildren,
                                       int norm, int iUniform) {
    DASHER_ASSERT(!vChildren.empty());

    if (vChildren.size() == 1) {
        vChildren[0].second = norm;
        return;
    }

    const CPPMContext* ppmcontext = reinterpret_cast<const CPPMContext*>(context);

    //  DASHER_ASSERT(m_setContexts.count(ppmcontext) > 0);

    //  probs.resize(iNumSymbols);

    unsigned int iToSpend = norm;
    unsigned int iUniformLeft = iUniform;

    // TODO: Sort out zero symbol case

    // Reproduce iterative calculations with SCENode trie

    // ACL following loop distributes the part of the probability mass assigned the uniform distribution.
    //  In Will's code, it assigned 0 to the first entry, then split evenly among the rest...seems wrong?!
    int i = 0;
    for (std::vector<std::pair<symbol, unsigned int>>::iterator it = vChildren.begin(); it != vChildren.end(); it++) {
        DASHER_ASSERT(it->first > 0 && it->first < GetSize()); // i.e., is valid CH symbol
        it->second = static_cast<unsigned int>(iUniformLeft / (vChildren.size() - i));
        iUniformLeft -= it->second;
        iToSpend -= it->second;
        i++;
    }

    DASHER_ASSERT(iUniformLeft == 0);

    int alpha = m_pSettingsStore->GetLongParameter(LP_LM_ALPHA);
    int beta = m_pSettingsStore->GetLongParameter(LP_LM_BETA);

    int* vCounts = new int[vChildren.size()]; // num occurrences of symbol at same index in vChildren
    std::fill(vCounts, vCounts + vChildren.size(), 0);

    // new code
    for (CPPMnode* pTemp = ppmcontext->head; pTemp; pTemp = pTemp->vine) {
        int iTotal = 0, j = 0;
        for (std::vector<std::pair<symbol, unsigned int>>::const_iterator it = vChildren.begin(); it != vChildren.end();
             it++, j++) {
            if (CPPMnode* pFound = pTemp->find_symbol(it->first)) {
                iTotal += vCounts[j] = pFound->count; // double assignment
            } else
                vCounts[j] = 0;
        }

        if (iTotal) {
            unsigned int size_of_slice = iToSpend;

            int k = 0;
            for (std::vector<std::pair<symbol, unsigned int>>::iterator it = vChildren.begin(); it != vChildren.end();
                 it++, k++) {
                if (vCounts[k]) {
                    unsigned int p =
                        static_cast<myint>(size_of_slice) * (100 * vCounts[k] - beta) / (100 * iTotal + alpha);
                    it->second += p;
                    iToSpend -= p;
                }
            }
        }
    }
    delete[] vCounts;
    // code

    //      std::ostringstream str;
    //      for (sym=0;sym<modelchars;sym++)
    //              str << probs[sym] << " ";
    //      str << std::endl;
    //      DASHER_TRACEOUTPUT("probs %s",str.str().c_str());

    //      std::ostringstream str2;
    //      for (sym=0;sym<modelchars;sym++)
    //              str2 << valid[sym] << " ";
    //      str2 << std::endl;
    //      DASHER_TRACEOUTPUT("valid %s",str2.str().c_str());

    // allow for rounding error by distributing the leftovers evenly amongst all elements...
    //  (ACL: previous code assigned nothing to element. Why? - I'm guessing due to confusion
    //   with other LM code where the first element of the probability array was a dummy,
    //  storing 0 probability mass assigned to the 'root symbol' - not the case here!)
    unsigned int p = iToSpend / static_cast<unsigned int>(vChildren.size());
    for (std::vector<std::pair<symbol, unsigned int>>::iterator it = vChildren.begin(); it != vChildren.end(); it++) {
        it->second += p;
        iToSpend -= p;
    }
    int iLeft = static_cast<int>(vChildren.size()) - 1;

    for (std::vector<std::pair<symbol, unsigned int>>::iterator it = vChildren.begin() + 1; it != vChildren.end();
         it++) {

        unsigned int pRem = iToSpend / iLeft;
        it->second += pRem;
        --iLeft;
        iToSpend -= pRem;
    }

    DASHER_ASSERT(iToSpend == 0);
}

// ACL this was Will's original "GetPYProbs" method - explicitly called instead of GetProbs
//  by an explicit cast to PPMPYLanguageModel whenever MandarinDasher was activated. Renaming
//  to GetProbs causes the normal (virtual) call to come straight here without any special-casing...
void CPPMPYLanguageModel::GetProbs(Context context, std::vector<unsigned int>& probs, int norm, int iUniform) const {
    const CPPMContext* ppmcontext = reinterpret_cast<const CPPMContext*>(context);

    /*
     CPPMPYnode * pNode = m_pRoot->child;

      while(pNode){
      std::cout<<"Next Symbol: "<<pNode->symbol<<"   ";
      pNode = pNode->next;
    }
      std::cout<<" "<<std::endl;
    */
    //  DASHER_ASSERT(m_setContexts.count(ppmcontext) > 0);

    int iNumSymbols = m_iNumPYsyms + 1;

    probs.resize(iNumSymbols);

    std::vector<bool> exclusions(iNumSymbols);

    unsigned int iToSpend = norm;
    unsigned int iUniformLeft = iUniform;

    // TODO: Sort out zero symbol case
    probs[0] = 0;
    exclusions[0] = false;

    int i;
    for (i = 1; i < iNumSymbols; i++) {
        probs[i] = iUniformLeft / (iNumSymbols - i);
        iUniformLeft -= probs[i];
        iToSpend -= probs[i];
        exclusions[i] = false;
    }

    DASHER_ASSERT(iUniformLeft == 0);

    //  bool doExclusion = GetLongParameter( LP_LM_ALPHA );
    bool doExclusion = 0; // FIXME

    int alpha = m_pSettingsStore->GetLongParameter(LP_LM_ALPHA);
    int beta = m_pSettingsStore->GetLongParameter(LP_LM_BETA);

    for (CPPMnode* pTemp = ppmcontext->head; pTemp; pTemp = pTemp->vine) {
        int iTotal = 0;
        const std::map<symbol, unsigned short int>& pychild(dynamic_cast<CPPMPYnode*>(pTemp)->pychild);

        for (std::map<symbol, unsigned short int>::const_iterator it = pychild.begin(); it != pychild.end(); it++) {
            if (!(exclusions[it->first] && doExclusion)) iTotal += it->second;
        }

        if (iTotal) {
            unsigned int size_of_slice = iToSpend;

            for (std::map<symbol, unsigned short int>::const_iterator it = pychild.begin(); it != pychild.end(); it++) {
                if (!(exclusions[it->first] && doExclusion)) {
                    exclusions[it->first] = 1;

                    unsigned int p =
                        static_cast<myint>(size_of_slice) * (100 * it->second - beta) / (100 * iTotal + alpha);

                    probs[it->first] += p;
                    iToSpend -= p;
                }
                //                              Usprintf(debug,TEXT("sym %u counts %d p %u tospend %u
                //                              \n"),sym,s->count,p,tospend); DebugOutput( debug);
            }
        }
    }

    unsigned int size_of_slice = iToSpend;
    int symbolsleft = 0;

    for (i = 1; i < iNumSymbols; i++)
        if (!(exclusions[i] && doExclusion)) symbolsleft++;

    //      std::ostringstream str;
    //      for (sym=0;sym<modelchars;sym++)
    //              str << probs[sym] << " ";
    //      str << std::endl;
    //      DASHER_TRACEOUTPUT("probs %s",str.str().c_str());

    //      std::ostringstream str2;
    //      for (sym=0;sym<modelchars;sym++)
    //              str2 << valid[sym] << " ";
    //      str2 << std::endl;
    //      DASHER_TRACEOUTPUT("valid %s",str2.str().c_str());

    for (i = 1; i < iNumSymbols; i++) {
        if (!(exclusions[i] && doExclusion)) {
            unsigned int p = size_of_slice / symbolsleft;
            probs[i] += p;
            iToSpend -= p;
        }
    }

    int iLeft = iNumSymbols - 1;

    for (int j = 1; j < iNumSymbols; ++j) {
        unsigned int p = iToSpend / iLeft;
        probs[j] += p;
        --iLeft;
        iToSpend -= p;
    }

    DASHER_ASSERT(iToSpend == 0);
}

/////////////////////////////////////////////////////////////////////

// Do _not_ move on the context...
void CPPMPYLanguageModel::LearnPYSymbol(Context c, int pysym) {
    // Ignore attempts to add the root symbol
    if (pysym == 0) return;

    DASHER_ASSERT(pysym > 0 && pysym <= m_iNumPYsyms);
    CPPMPYLanguageModel::CPPMContext& context = *reinterpret_cast<CPPMContext*>(c);

    /*   CPPMPYnode * pNode = m_pRoot->child;

       while(pNode){
      std::cout<<"Next Symbol: "<<pNode->symbol<<"   ";
      pNode = pNode->next;
      }
       std::cout<<" "<<std::endl;
    */

    for (CPPMnode* pNode = context.head; pNode; pNode = pNode->vine) {
        if (dynamic_cast<CPPMPYnode*>(pNode)->pychild[pysym]++) {
            // count non-zero before increment, i.e. sym already present
            if (bUpdateExclusion) break;
        }
    }

    // no context order increase
    // context.order++;
}

CPPMPYLanguageModel::CPPMPYnode* CPPMPYLanguageModel::makeNode(int sym) {
    CPPMPYnode* res = m_NodeAlloc.Alloc();
    res->sym = sym;
    ++NodesAllocated;
    return res;
}

// Mandarin - PY not enabled for these read-write functions
bool CPPMPYLanguageModel::WriteToFile(std::string strFilename) {
    return false;
}

// Mandarin - PY not enabled for these read-write functions
bool CPPMPYLanguageModel::ReadFromFile(std::string strFilename) {
    return false;
}
