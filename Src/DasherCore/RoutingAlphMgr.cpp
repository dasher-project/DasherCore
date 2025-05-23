//
//  RoutingAlphMgr.cpp
//  Dasher
//
//  Created by Alan Lawrence on 13/12/11.
//  Copyright 2011 Cambridge University. All rights reserved.
//

#include "RoutingAlphMgr.h"

#include <I18n.h>

#include "DasherInterfaceBase.h"
#include "LanguageModelling/RoutingPPMLanguageModel.h"
using namespace Dasher;


CRoutingAlphMgr::CRoutingAlphMgr(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CNodeCreationManager *pNCManager, const CAlphInfo *pAlphabet)
: CAlphabetManager(pSettingsStore, pInterface, pNCManager, pAlphabet) {
  
  DASHER_ASSERT(pAlphabet->m_iConversionID==3 || pAlphabet->m_iConversionID==4);
}

void CRoutingAlphMgr::InitMap() {
  m_vBaseSyms.reserve(m_pAlphabet->iEnd); m_vBaseSyms.push_back(0); //base for unknown route = unknown!
  m_vRoutes.push_back(std::set<symbol>()); //unknown base symbol has no routes
  for (int i=1; i<m_pAlphabet->iEnd; i++) {
    symbol s = m_map.Get(m_pAlphabet->GetText(i));
    if (s==0) {
      s=static_cast<Dasher::symbol>(m_vRoutes.size());
      m_vRoutes.push_back(std::set<symbol>());
      m_map.Add(m_pAlphabet->GetText(i),s);
    }
    m_vBaseSyms.push_back(s);
    m_vRoutes[s].insert(i);
  }
  m_vGroupsByRoute.resize(m_vBaseSyms.size());
  std::vector<const SGroupInfo *> vGroups;
  DASHER_ASSERT(!m_pAlphabet->pNext);
  vGroups.push_back(m_pAlphabet->pChild);
  while (!vGroups.empty()) {
    const SGroupInfo *g(vGroups.back()); vGroups.pop_back();
    if (!g) continue;
    for (int i=g->iStart; i<g->iEnd; i++) m_vGroupsByRoute[i]=g;
    vGroups.push_back(g->pNext);
    vGroups.push_back(g->pChild);
  }
}

void CRoutingAlphMgr::CreateLanguageModel() {
  m_pLanguageModel = new CRoutingPPMLanguageModel(m_pSettingsStore, &m_vBaseSyms, &m_vRoutes, m_pAlphabet->m_iConversionID == CAlphInfo::RoutingContextSensitive);
}

std::string CRoutingAlphMgr::CRoutedSym::trainText() {
  const std::set<symbol> &routes(mgr()->m_vRoutes[mgr()->m_vBaseSyms[iSymbol]]);
  DASHER_ASSERT(routes.count(iSymbol));
  std::string t=CSymbolNode::trainText();
  if (routes.size()!=1)
    if (const SGroupInfo *g = mgr()->m_vGroupsByRoute[iSymbol])
      return mgr()->m_pAlphabet->m_strConversionTrainStart + g->strName + mgr()->m_pAlphabet->m_strConversionTrainStop + t;
  return t;
}

CRoutingAlphMgr::CRoutedSym::CRoutedSym(int iOffset, CDasherScreen::Label *pLabel, CRoutingAlphMgr *pMgr, symbol iSymbol)
: CSymbolNode(iOffset, pLabel, pMgr, iSymbol) {
};


CAlphNode *CRoutingAlphMgr::CreateSymbolRoot(int iOffset, CLanguageModel::Context ctx, symbol sym) {
  //sym is from the map, so a base symbol. It's at the end of the context,
  // TODO unless this is the completely-empty context,
  // so ask the LM for which way it's most likely to have been entered
  sym = static_cast<CRoutingPPMLanguageModel*>(m_pLanguageModel)->GetBestRoute(ctx);
  return new CRoutedSym(iOffset, m_vLabels[sym], this, sym);
}

CDasherNode *CRoutingAlphMgr::CreateSymbolNode(CAlphNode *pParent, symbol iSymbol) {

  int iNewOffset = pParent->offset()+1;
  if (m_pAlphabet->GetText(iSymbol)=="\r\n") iNewOffset++;
  CSymbolNode *pAlphNode = new CRoutedSym(iNewOffset, m_vLabels[iSymbol], this, iSymbol);
  
  pAlphNode->iContext = m_pLanguageModel->CloneContext(pParent->iContext);
  
  //namely, we want to enter only the BASE symbol into the LM, not the route
  // (which would be out of range):
  m_pLanguageModel->EnterSymbol(pAlphNode->iContext, m_vBaseSyms[iSymbol]);
  // (Unfortunately, we can't make EnterSymbol take route numbers, because
  // it has base symbols passed to it from the alphabet map)
  return pAlphNode;

}

CRoutingAlphMgr::CRoutingTrainer::CRoutingTrainer(CMessageDisplay *pMsgs, CRoutingAlphMgr *pMgr)
: CTrainer(pMsgs, pMgr->m_pLanguageModel, pMgr->m_pAlphabet, &pMgr->m_map), m_pMgr(pMgr) {
  
  m_iStartSym=0;  
  std::vector<symbol> trainStartSyms;
  m_pAlphabet->GetSymbols(trainStartSyms, m_pInfo->m_strConversionTrainStart);
  if (trainStartSyms.size()==1)
    m_iStartSym = trainStartSyms[0];
  else
    m_pMsgs->FormatMessage("Warning: faulty alphabet definition: training-start delimiter %s must be a single unicode character. May be unable to process training file.",
                                     m_pInfo->m_strConversionTrainStart.c_str());
}

symbol CRoutingAlphMgr::CRoutingTrainer::getRoute(bool bHaveRoute, const std::string &strRoute, symbol baseSym) {  
  const std::set<symbol> &candidates(m_pMgr->m_vRoutes.at(baseSym));
  std::set<symbol> named;
  for (std::set<symbol>::iterator it=candidates.begin(); it!=candidates.end(); it++)
    if (const SGroupInfo *g=m_pMgr->m_vGroupsByRoute[*it])
      if (g->strName == strRoute)
        named.insert(*it);
  //if no name was given, but a single group with no name exists, use it!
  if (named.size()==1) return *(named.begin());
  //otherwise, we will not learn a route - but this is fine, we can learn
  // that later more-or-less independently
  
  if (bHaveRoute) {
    m_pMsgs->FormatMessage((named.size()==0)
                                       ? "Warning: training file contains character '%s' as member of group '%s', but no group of that name contains the character. Ignoring group specifier."
                                       : "Warning: training file contains character '%s' as member of group '%s', but alphabet contains several such groups. Dasher will not be able to learn how you want to write this character.",
                                         m_pInfo->GetDisplayText(baseSym).c_str(),
                                         strRoute.c_str());
  }
  // don't flag a problem if no route specified
  
  return 0;
}

void CRoutingAlphMgr::CRoutingTrainer::Train(CAlphabetMap::SymbolStream &syms) {
  CLanguageModel::Context trainContext = m_pLanguageModel->CreateEmptyContext();
  
  std::string strRoute; bool bHaveRoute(false);
  for (symbol sym; (sym=syms.next(m_pAlphabet))!=-1;) {
    if (sym == m_iStartSym) {
      if (sym!=0 || syms.peekBack()==m_pInfo->m_strConversionTrainStart) {
        if (bHaveRoute)
          m_pMsgs->FormatMessage("Warning: in training file, annotation '<%s>' is followed by another annotation and will be ignored",
                                           strRoute.c_str());
        strRoute.clear(); bHaveRoute=true;
        for (std::string s; (s=syms.peekAhead()).length(); strRoute+=s) {
          syms.next(m_pAlphabet);
          if (s==m_pInfo->m_strConversionTrainStop) break;
        }
        continue; //read next, hopefully a CH (!)
      } //else, unknown symbol, but does not match pinyin delimiter; fallthrough
    }
    if (readEscape(trainContext, sym, syms)) continue; //TODO warn if py lost?
                                                       //OK, sym is a (CH) symbol to learn.
    if (sym) {
      if (symbol route = getRoute(bHaveRoute, strRoute, sym))
        m_pLanguageModel->LearnSymbol(trainContext, route);
      else
        static_cast<CRoutingPPMLanguageModel*>(m_pLanguageModel)->LearnBaseSymbol(trainContext, sym);
    } //else, silently drop - as standard CTrainer
    bHaveRoute=false; strRoute.clear();
  }
  m_pLanguageModel->ReleaseContext(trainContext);
}


CTrainer *CRoutingAlphMgr::GetTrainer() {
  //We pass in the pinyin alphabet to define the context-switch escape character, and the default context.
  // Although the default context will be symbolified via the _chinese_ alphabet, this seems reasonable
  // as it is the Pinyin alphabet which defines the conversion mapping (i.e. m_strConversionTarget!)
  return new CRoutingTrainer(m_pInterface, this);
}
