// MandarinAlphMgr.cpp
//
// Copyright (c) 2009 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "MandarinAlphMgr.h"

#include <algorithm>
#include <I18n.h>

#include "LanguageModelling/PPMPYLanguageModel.h"
#include "DasherInterfaceBase.h"
#include "DasherNode.h"
#include "NodeCreationManager.h"

#include <string.h>

#include <vector>
#include <sstream>
#include <cstdint>

using namespace Dasher;

CMandarinAlphMgr::CMandarinAlphMgr(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CNodeCreationManager *pNCManager, const CAlphInfo *pAlphabet)
    : CAlphabetManager(pSettingsStore, pInterface, pNCManager, pAlphabet), m_pPYgroups(nullptr), m_iCHpara(0),
      m_pScreen(nullptr)
{
    DASHER_ASSERT(pAlphabet->m_iConversionID==2);
}

void CMandarinAlphMgr::InitMap() {
  m_vCHtext.resize(1);
  m_vCHdisplayText.resize(1);
  m_vGroupsByConversion.resize(1);
  m_vConversionsByGroup.resize(1);
  m_vGroupNames.resize(1);
  
  //Scan original group tree, identifying PY groups and rehashing characters
  m_pPYgroups = makePYgroup(m_pAlphabet);

  //add in space and paragraph to end of main PY group...
  char additonalChars[2] = {' ', '\n'};
  for (const char i : additonalChars)
  {
    //add a PY mapping to a single CH-character (already rehashed), i.e. the space/para
    int hashed = m_map.GetSingleChar(i);
    DASHER_ASSERT(hashed);
    m_vGroupsByConversion[hashed].insert(static_cast<int>(m_vConversionsByGroup.size())); //identifies the new PY sound
    m_vConversionsByGroup.push_back(std::vector<symbol>(1,hashed));
    m_pPYgroups->iEnd++; m_pPYgroups->iNumChildNodes++;
  }
}

SGroupInfo *CMandarinAlphMgr::makePYgroup(const SGroupInfo *in) {
  if (!in) return NULL;
  SGroupInfo *ret = new SGroupInfo(*in);
  ret->iStart = static_cast<int>(m_vConversionsByGroup.size());//i.e. #py sounds found so far
  
  //process any symbols that are direct children of the group (not descendants)
  ret->iNumChildNodes=0; //we'll have to do a recount
  const SGroupInfo *oldCh = in->pChild;
  for (int i=in->iStart; i<in->iEnd; ) {
    if (!oldCh || i<oldCh->iStart) {
      //found symbol...first, rehash:
      const std::string &text(m_pAlphabet->GetText(i));
      int hashed=m_map.Get(text);
      if (!hashed) {
        //unicode char not seen already, allocate new symbol number
        hashed = static_cast<int>(m_vCHtext.size());
        m_vCHtext.push_back(text);
        m_vCHdisplayText.push_back(m_pAlphabet->GetDisplayText(i));
        if (m_pAlphabet->SymbolPrintsNewLineCharacter(i))
          m_map.AddParagraphSymbol(m_iCHpara=hashed);
        else
          m_map.Add(text,hashed);
        m_vGroupsByConversion.push_back(std::set<symbol>());
      }
      //now, put in PY-group...
      //if (i!=m_pAlphabet->GetSpaceSymbol() && i!=m_pAlphabet->GetParagraphSymbol()) { // Pretty sure that these characters do not exist in any group, so this should never happen in my understanding
        if (m_vConversionsByGroup.size() == ret->iStart) {
          //First symbol that is directly child of group. Allocate index...
          ret->iNumChildNodes++;
          m_vGroupNames.push_back(in->strName);
          m_vConversionsByGroup.push_back(std::vector<symbol>());
        }
        DASHER_ASSERT(m_vGroupNames.size() > ret->iStart);
        DASHER_ASSERT(m_vConversionsByGroup.size() > ret->iStart);
        m_vConversionsByGroup.back().push_back(hashed);
        m_vGroupsByConversion[hashed].insert(ret->iStart);
      //} //space and para we will put in their own/different groups, later...
      i++;
    } else {
      //subgroup; skip over
      i=oldCh->iEnd; oldCh=oldCh->pNext;
      ret->iNumChildNodes++;
    }
  }
  //direct children done. process indirect children, i.e. subgroups
  ret->pChild=makePYgroup(ret->pChild);
  ret->iEnd = static_cast<int>(m_vConversionsByGroup.size()); //record all indices allocated to this group and descendants
  ret->pNext = makePYgroup(ret->pNext);
  return ret;
}

void CMandarinAlphMgr::MakeLabels(CDasherScreen *pScreen) {
  //keep the screen to create labels lazily...
  m_pScreen = pScreen;
  
  //this copies the entire group tree to the new indices
  // and makes any labels req'd for conversion roots
  // (i.e. captions of groups, containing said root, that are elided)
  CAlphabetManager::MakeLabels(pScreen);
}

SGroupInfo *CMandarinAlphMgr::copyGroups(const SGroupInfo *in, CDasherScreen *pScreen) {
  return CAlphabetManager::copyGroups(in==m_pAlphabet ? m_pPYgroups : in, pScreen);
}
const std::string &CMandarinAlphMgr::GetLabelText(symbol i) const {
  static std::string n="";
  return n;
}

CMandarinAlphMgr::~CMandarinAlphMgr() {
  for (std::vector<CDasherScreen::Label *>::iterator it=m_vCHLabels.begin(); it!=m_vCHLabels.end(); it++)
    delete *it;
  delete m_pPYgroups;
}

void CMandarinAlphMgr::CreateLanguageModel() {
  //std::cout<<"CHALphabet size "<< pCHAlphabet->GetNumberTextSymbols(); [7603]
  //std::cout<<"Setting PPMPY model"<<std::endl;
  m_pLanguageModel = new CPPMPYLanguageModel(m_pSettingsStore, static_cast<int>(m_vGroupsByConversion.size())-1, static_cast<int>(m_vConversionsByGroup.size())-1);
}

CMandarinAlphMgr::CMandarinTrainer::CMandarinTrainer(CMessageDisplay *pMsgs, CMandarinAlphMgr *pMgr)
: CTrainer(pMsgs, pMgr->m_pLanguageModel, pMgr->m_pAlphabet, &pMgr->m_map), m_pMgr(pMgr) {
  //We pass in the alphabet to define the context-switch escape character, and the default context.

  m_iStartSym=0;  
  std::vector<symbol> trainStartSyms;
  m_pAlphabet->GetSymbols(trainStartSyms, m_pInfo->m_strConversionTrainStart);
  if (trainStartSyms.size()==1)
    m_iStartSym = trainStartSyms[0];
  else
    m_pMsgs->FormatMessage("Warning: faulty alphabet definition: training-start delimiter %s must be a single unicode character. May be unable to process training file.",
                                     m_pInfo->m_strConversionTrainStart.c_str());
}

symbol CMandarinAlphMgr::CMandarinTrainer::getPYsym(bool bHavePy, const std::string &strPy, symbol symCh) {
  const std::set<symbol> &posPY(m_pMgr->m_vGroupsByConversion[symCh]);
  if (posPY.size()==1) {
    //only one possibility; so we'll use it, but maybe flag.
    symbol pySym = *(posPY.begin());
    if (bHavePy && m_pMgr->m_vGroupNames[pySym] != strPy)
      m_pMsgs->FormatMessage("Warning: training file contains character '%s' as member of group '%s', but no group of that name contains the character; ignoring group specifier",
                                         m_pInfo->GetDisplayText(symCh).c_str(),
                                         strPy.c_str());
    return pySym;
  }
  std::set<symbol> withName;
  if (bHavePy) {
    for (std::set<symbol>::iterator it = posPY.begin(); it!=posPY.end(); it++)
      if (m_pMgr->m_vGroupNames[*it] == strPy)
        withName.insert(*it);  
    if (withName.size()==1) return *(withName.begin());
    else
      m_pMsgs->FormatMessage((withName.empty())
                                         ? "Warning: training file contains character '%s' as member of group '%s', but no group of that name contains the character. Dasher will not be able to learn how you want to write this character."
                                         : "Warning: training file contains character '%s' as member of group '%s', but alphabet contains several such groups. Dasher will not be able to learn how you want to write this character.",
                                         m_pInfo->GetDisplayText(symCh).c_str(),
                                         strPy.c_str());
  }
  return 0;
}

void CMandarinAlphMgr::CMandarinTrainer::Train(CAlphabetMap::SymbolStream &syms) {
  CLanguageModel::Context trainContext = m_pLanguageModel->CreateEmptyContext();
  //store a set of CH symbols which need annotations but have appeared without them
  // in this training file. We do this to cut down on the number of error messages
  // that you get if you use an unannotated training file - this is suboptimal,
  // so we want to warn the user, but we don't want to make Dasher unusable by
  // flooding them with error messages.
  std::set<symbol> unannotated;
  std::string strPy; bool bHavePy(false);
  for (symbol sym; (sym=syms.next(m_pAlphabet))!=-1;) {
    if (sym == m_iStartSym) {
      if (sym!=0 || syms.peekBack()==m_pInfo->m_strConversionTrainStart) {
        if (bHavePy)
          m_pMsgs->FormatMessage("Warning: in training file, annotation '<%s>' is followed by another annotation and will be ignored",
                                           strPy.c_str());
        strPy.clear(); bHavePy=true;
        for (std::string s; (s=syms.peekAhead()).length(); strPy+=s) {
          syms.next(m_pAlphabet);
          if (s==m_pInfo->m_strConversionTrainStop) break;
        }
        continue; //read next, hopefully a CH (!)
      } //else, unknown symbol, but does not match pinyin delimiter; fallthrough
    }
    if (readEscape(trainContext, sym, syms)) continue; //TODO warn if py lost?
    //OK, sym is a (CH) symbol to learn.
    if (sym) {
      if (symbol pySym = getPYsym(bHavePy, strPy, sym))
          static_cast<CPPMPYLanguageModel*>(m_pLanguageModel)->LearnPYSymbol(trainContext, pySym);
      else if (!bHavePy) unannotated.insert(sym); //no PY and unannotated -> warn user
      m_pLanguageModel->LearnSymbol(trainContext, sym);
    } //else, silently drop - as CTrainer - TODO could learn PY anyway???
    bHavePy=false; strPy.clear();
  }
  if (unannotated.size()) {
    // AM_GLIB_GNU_GETTEXT sets HAVE_GETTEXT if it finds a version of gettext
    // which includes ngettext() - there is no separate HAVE_NGETTEXT.
    ///TRANSLATORS: first string will be the filename; after the end of the string,
    /// some number of output (e.g. Chinese) characters will be appended,
    /// the number of which is the integer here
#ifdef HAVE_GETTEXT
    const char* msg = ngettext("In file %s, the following %i symbol appeared without annotations saying how it should be entered, but it can be entered in several ways. Dasher will not be able to learn how you want to enter this symbol:",
      "In file %s, the following %i symbols appeared without annotations saying how they should be entered, but each can be entered in several ways. Dasher will not be able to learn how you want to enter these symbols:",
      unannotated.size());
#else
    const char* msg = _("In file %s, the following %i symbols appeared without annotations saying how they should be entered, but each can be entered in several ways. Dasher will not be able to learn how you want to enter these symbols:");
#endif
    const size_t buflen =  strlen(msg) + GetDesc().length() + 10;
    char* buf(new char[buflen]);
    snprintf(buf, buflen, msg, GetDesc().c_str(), unannotated.size());
    std::ostringstream withChars;
    withChars << msg;
    for (std::set<symbol>::iterator it = unannotated.begin(); it!=unannotated.end(); it++)
      withChars << " " << m_pInfo->GetDisplayText(*it);
    m_pMsgs->Message(withChars.str(), true);
  }
  m_pLanguageModel->ReleaseContext(trainContext);
}


CTrainer *CMandarinAlphMgr::GetTrainer() {
  return new CMandarinTrainer(m_pInterface, this);
}

CAlphNode *CMandarinAlphMgr::CreateSymbolRoot(int iOffset, CLanguageModel::Context ctx, symbol chSym) {
  return new CMandSym(iOffset, this, chSym, 0);
}

CDasherNode *CMandarinAlphMgr::CreateSymbolNode(CAlphNode *pParent, symbol iSymbol) {
  
  //For every PY index (group; = syllable+tone or "punctuation"),
  // m_pConversionsByGroup identifies the possible chinese-alphabet symbols
  // that have that syll+tone; a CConvRoot thus offers the choice between them.
  //However, for e.g. punctuation, there may be only one such CH symbol, in which
  // case we can create the symbol directly, bypassing the CConvRoot; EXCEPT,
  // in cases where the CConvRoot provides a place to put some part of the group
  // label (specific to that symbol, so kinda redundant, but we want to keep it
  // for consistency of display).
  std::vector<symbol> &convs(m_vConversionsByGroup[iSymbol]);
  
  if (convs.size()>1 || m_vLabels[iSymbol])
    return CreateConvRoot(pParent, iSymbol);
  //elide CConvRoot...
  return CreateCHSymbol(pParent,pParent->iContext, *(convs.begin()), iSymbol);
}

CMandarinAlphMgr::CConvRoot *CMandarinAlphMgr::CreateConvRoot(CAlphNode *pParent, symbol iPYsym) {
  
  // the same offset as we've still not entered/selected a symbol (leaf);
  // Color is always 9 so ignore iBkgCol
  CConvRoot *pConv = new CConvRoot(pParent->offset(), this, iPYsym);
    
  // and use the same context too (pinyin syll+tone is _not_ used as part of the LM context)
  pConv->iContext = m_pLanguageModel->CloneContext(pParent->iContext);
  return pConv;
}

CMandarinAlphMgr::CConvRoot::CConvRoot(int iOffset, CMandarinAlphMgr *pMgr, symbol pySym)
: CAlphBase(iOffset, pMgr->m_vLabels[pySym], pMgr), m_pySym(pySym) {
  //We do sometimes create CConvRoots with only one child, where we
  // need them to display a label...
  DASHER_ASSERT(pMgr->m_vConversionsByGroup[pySym].size()>1
                || pMgr->m_vLabels[pySym]);
  //colour + label from ConversionManager.
}

int CMandarinAlphMgr::CConvRoot::ExpectedNumChildren() {
  return static_cast<int>(mgr()->m_vConversionsByGroup[m_pySym].size());
}

void CMandarinAlphMgr::CConvRoot::PopulateChildren() {
  PopulateChildrenWithExisting(NULL);
}

void CMandarinAlphMgr::CConvRoot::PopulateChildrenWithExisting(CMandSym *existing) {
  if (m_vChInfo.empty()) {
    mgr()->GetConversions(m_vChInfo,m_pySym, iContext);
  }
  
  int iCum(0);
  
  // Finally loop through and create the children
  for (std::vector<std::pair<symbol, unsigned int> >::const_iterator it = m_vChInfo.begin(); it!=m_vChInfo.end(); it++) {
    //      std::cout << "Current scec: " << pCurrentSCEChild << std::endl;
    const unsigned int iLbnd(iCum), iHbnd(iCum + it->second);
    
    iCum = iHbnd;
    CMandSym *pNewNode = (existing)
      ? existing->RebuildCHSymbol(this, it->first)
      : mgr()->CreateCHSymbol(this, this->iContext, it->first, m_pySym);
    pNewNode->Reparent(this, iLbnd, iHbnd);
  }
}

CMandarinAlphMgr::CMandSym *CMandarinAlphMgr::CreateCHSymbol(CDasherNode *pParent, CLanguageModel::Context iContext, symbol iCHsym, symbol iPYparent) {
  int iNewOffset = pParent->offset()+1;
  if (m_vCHtext[iCHsym] == "\r\n") iNewOffset++;
  CMandSym *pNewNode = new CMandSym(iNewOffset, this, iCHsym, iPYparent);
  pNewNode->iContext = m_pLanguageModel->CloneContext(iContext);
  m_pLanguageModel->EnterSymbol(pNewNode->iContext, iCHsym);
  return pNewNode;
}

CDasherNode *CMandarinAlphMgr::CConvRoot::RebuildSymbol(CAlphNode *pParent, symbol iSym) {
  if (iSym == m_pySym) return this;
  return CAlphBase::RebuildSymbol(pParent, iSym);
}

bool CMandarinAlphMgr::CConvRoot::isInGroup(const SGroupInfo *pGroup) {
  return pGroup->iStart <= m_pySym && pGroup->iEnd > m_pySym;
}

const ColorPalette::Color& CMandarinAlphMgr::CConvRoot::getLabelColor(const ColorPalette* colorPalette)
{
    return colorPalette->GetNamedColor(NamedColor::defaultLabel);
}

const ColorPalette::Color& CMandarinAlphMgr::CConvRoot::getOutlineColor(const ColorPalette* colorPalette)
{
     return colorPalette->GetNamedColor(NamedColor::defaultOutline);
}

const ColorPalette::Color& CMandarinAlphMgr::CConvRoot::getNodeColor(const ColorPalette* colorPalette)
{
     return colorPalette->GetNamedColor(NamedColor::conversionNode);
}

void CMandarinAlphMgr::CConvRoot::SetFlag(int iFlag, bool bValue) {
  if (iFlag==NF_COMMITTED && bValue && !GetFlag(NF_COMMITTED)
      && !GetFlag(NF_GAME) && mgr()->m_pSettingsStore->GetBoolParameter(BP_LM_ADAPTIVE)) {
    //CConvRoot's context is the same as parent's context (no symbol yet!),
    // i.e. is the context in which the pinyin was predicted.
    static_cast<CPPMPYLanguageModel *>(mgr()->m_pLanguageModel)->LearnPYSymbol(iContext, m_pySym);
  }
  CDasherNode::SetFlag(iFlag,bValue);
}

// For sorting pair<symbol, probability>s into descending order
// (most probable first, hence sense of >)
bool CompSecond(const std::pair<symbol,unsigned int> &a, const std::pair<symbol,unsigned int> &b) {
  return a.second>b.second;
}

void CMandarinAlphMgr::GetConversions(std::vector<std::pair<symbol,unsigned int> > &vChildren, symbol pySym, Dasher::CLanguageModel::Context context) {

  const std::vector<symbol> &convs(m_vConversionsByGroup[pySym]);

  //Symbols which we are including with the probabilities predicted by the LM.
  // We do this for the most probable symbols first, up to at least BY_PY_PROB_SORT_THRES
  // (a percentage); after this, the remaining probability mass is distributed
  // uniformly between the remaining symbols, which are kept in alphabet order (after the
  // more probable ones).
  //Two degenerate cases: PROB_SORT_THRES=0 => all (legal) ch symbols predicted uniformly
  // PROB_SORT_THRES=100 => all symbols put into probability order
  std::set<symbol> haveProbs;
  uint64_t iRemaining(CDasherModel::NORMALIZATION);
  
  if (long percent=m_pSettingsStore->GetLongParameter(LP_PY_PROB_SORT_THRES)) {
    const uint64_t iNorm(iRemaining);
    const unsigned int uniform(static_cast<unsigned int>((m_pSettingsStore->GetLongParameter(LP_UNIFORM)*iNorm)/1000));
    
    //Set up list of symbols with blank probability entries...
    for(std::vector<symbol>::const_iterator it = convs.begin(); it != convs.end(); ++it) {
      vChildren.push_back(std::pair<symbol, unsigned int>(*it,0));
    }
    
    //Then call LM to fill in the probs, passing iNorm and uniform directly -
    // GetPartProbs distributes the last param between however elements there are in vChildren...
    static_cast<CPPMPYLanguageModel *>(m_pLanguageModel)->GetPartProbs(context, vChildren, static_cast<int>(iNorm), uniform);
  
    //std::cout<<"after get probs "<<std::endl;
  
    uint64_t sumProb=0;  
    for (std::vector<std::pair<symbol,unsigned int> >::const_iterator it = vChildren.begin(); it!=vChildren.end(); it++) {
      sumProb += it->second;
    }
    DASHER_ASSERT(sumProb==iNorm);
    
    //Sort all symbols into probability order (highest first)
    stable_sort(vChildren.begin(), vChildren.end(), CompSecond);
    if (percent>=100) return; //and that's what's required.
    
    //ok, some symbols as predicted by LM, others not...
    std::vector<std::pair<symbol,unsigned int> > probOrder;
    swap(vChildren, probOrder);
    const unsigned int stop(static_cast<unsigned int>(iNorm - (iNorm*percent)/100));//intermediate values are uint64
    for (std::vector<std::pair<symbol, unsigned int> >::iterator it=probOrder.begin(); iRemaining>stop; it++) {
      //assert: the remaining probability mass, divided by the remaining symbols,
      // (i.e. the probability mass each symbol would receive if we uniformed the rest)
      // must be less than the probability predicted by the LM (as we're processing
      // symbols in decreasing order)
      DASHER_ASSERT(iRemaining <= it->second*(convs.size()-vChildren.size()));
      vChildren.push_back(*it);
      haveProbs.insert(it->first);
      iRemaining-=it->second;
    }
  }
  //Now distribute iRemaining uniformly between all remaining symbols,
  // keeping them in alphabet order
  if (iRemaining) {
    unsigned int iEach(static_cast<unsigned int>(iRemaining) / (static_cast<unsigned int>(convs.size()) - static_cast<unsigned int>(haveProbs.size())));
    for (std::vector<symbol>::const_iterator it=convs.begin(); it!=convs.end(); it++) {
      if (haveProbs.count(*it)==0)
        vChildren.push_back(std::pair<symbol,unsigned int>(*it,iEach));
    }
    
    //account for rounding error by topping up
    DASHER_ASSERT(vChildren.size() == convs.size());
    iRemaining -= iEach * (convs.size() - haveProbs.size());
    unsigned int iLeft = static_cast<unsigned int>(vChildren.size());
    for (std::vector<std::pair<symbol, unsigned int> >::iterator it=vChildren.end(); iRemaining && it-- != vChildren.begin();) {
      const unsigned int p(static_cast<unsigned int>(iRemaining / iLeft));
      it->second+=p; iRemaining-=p; iLeft--;
    }
  }
}

CDasherScreen::Label *CMandarinAlphMgr::GetCHLabel(int iCHsym) {
  //TODO: LRU cache, keep down to some sensible #labels allocated?
  if (iCHsym>=m_vCHLabels.size()) {
    m_vCHLabels.resize(iCHsym+1);
  } else if (m_vCHLabels[iCHsym]) {
    return m_vCHLabels[iCHsym];
  }
  return m_vCHLabels[iCHsym] = m_pScreen->MakeLabel(m_vCHdisplayText[iCHsym]);
}

CMandarinAlphMgr::CMandSym::CMandSym(int iOffset, CMandarinAlphMgr *pMgr, symbol iSymbol, symbol pyParent)
: CSymbolNode(iOffset, pMgr->GetCHLabel(iSymbol), pMgr, iSymbol), m_pyParent(pyParent) {
}

CDasherNode *CMandarinAlphMgr::CMandSym::RebuildSymbol(CAlphNode *pParent, symbol iSymbol) {
  DASHER_ASSERT(m_pyParent!=0); //should have been computed in RebuildForwardsFromAncestor()
  if (iSymbol==m_pyParent) {
    //create the PY node that lead to this chinese
    if (mgr()->m_vConversionsByGroup[m_pyParent].size()==1) {
      DASHER_ASSERT( *(mgr()->m_vConversionsByGroup[m_pyParent].begin()) == this->iSymbol);
      return this;
    }
    //ok, will be a PY-to-Chinese conversion choice
    CConvRoot *pConv = mgr()->CreateConvRoot(pParent, iSymbol);
    pConv->PopulateChildrenWithExisting(this);
    return pConv;
  }
  return CAlphBase::RebuildSymbol(pParent, iSymbol);
}

bool CMandarinAlphMgr::CMandSym::isInGroup(const SGroupInfo *pGroup) {
  DASHER_ASSERT(m_pyParent!=0); //should have been computed in RebuildForwardsFromAncestor()
  //pinyin group contains the pinyin-"symbol"=CConvRoot which we want to be our parent...
  return pGroup->iStart <= m_pyParent && pGroup->iEnd > m_pyParent;
}

CMandarinAlphMgr::CMandSym *CMandarinAlphMgr::CMandSym::RebuildCHSymbol(CConvRoot *pParent, symbol iNewSym) {
  if (iNewSym == this->iSymbol) return this; //reuse existing node
  return mgr()->CreateCHSymbol(pParent, pParent->iContext, iNewSym, pParent->m_pySym);
}

void CMandarinAlphMgr::CMandSym::RebuildForwardsFromAncestor(CAlphNode *pNewNode) {
  if (m_pyParent==0) {
    std::set<symbol> &possiblePinyin(mgr()->m_vGroupsByConversion[iSymbol]);
    if (possiblePinyin.size() > 1) {
      //need to compare pinyin symbols; so compute probability of this (chinese) sym, for each:
      // i.e. P(pinyin) * P(this chinese | pinyin)
      const std::vector<unsigned int> &vPinyinProbs(*(pNewNode->GetProbInfo()));
      long bestProb=0; //of this chinese, over NORMALIZATION _squared_
      for (std::set<symbol>::iterator p_it = possiblePinyin.begin(); p_it!=possiblePinyin.end(); p_it++) {
        //compute probability of each chinese symbol for that pinyin (=by filtering)
        // context is the same as the ancestor = previous chinese, as pinyin not part of context
        std::vector<std::pair<symbol, unsigned int> > vChineseProbs;
        mgr()->GetConversions(vChineseProbs, *p_it, pNewNode->iContext);
        //now find us in that list
        long thisProb; //i.e. P(this pinyin) * P(this chinese | this pinyin)
        for (std::vector<std::pair<symbol,unsigned int> >::iterator c_it = vChineseProbs.begin(); ;) {
          if (c_it->first == iSymbol) {
            //found P(this chinese sym | pinyin). Compute overall...
            thisProb = c_it->second * vPinyinProbs[*p_it];
            break;
          }
          c_it++;
          DASHER_ASSERT(c_it!=vChineseProbs.end()); //gotta find this chinese sym somewhere...
        }
        //see if that works out better than for the other possible pinyin...
        if (thisProb > bestProb) {
          bestProb = thisProb;
          m_pyParent = *p_it;
        }
      }
    } else m_pyParent = *(possiblePinyin.begin());
  }
  CSymbolNode::RebuildForwardsFromAncestor(pNewNode);
}

const std::string &CMandarinAlphMgr::CMandSym::outputText() const {
  //Note this largely duplicates CAlphabetManager::CSymbolNode:outputText:
  if (iSymbol==mgr()->m_iCHpara && GetFlag(NF_SEEN)) {
    //Regardless of the platform's definition of a newline,
    // which is what we'd _output_, when reversing backwards, we represent
    // occurrences of _either_ \n or \r\n by a single paragraph symbol.
    DASHER_ASSERT(mgr()->m_pInterface->GetContext(offset(),1)=="\n");
    static std::string rn("\r\n"),n("\n"); //must store strings somewhere to return by reference!
    return (mgr()->m_pInterface->GetContext(offset()-1,2)=="\r\n") ? rn : n;
  }
  return mgr()->m_vCHtext[iSymbol];
}

std::string CMandarinAlphMgr::CMandSym::trainText() {
  //NF_COMMITTED should be in process of being set...
  DASHER_ASSERT(!GetFlag(NF_COMMITTED));
  //in which case, we should have a parent (if not, we would have to
  // have been built from string context, i.e. going backwards,
  // in which case we would be committed already)
  DASHER_ASSERT(Parent());
  //so the parent should have set our m_pyParent field...
  DASHER_ASSERT(m_pyParent);
  //if there is only one possible PY that might have lead to this CH sym, no need
  // to record that in the training text
  std::set<symbol> &py(mgr()->m_vGroupsByConversion[iSymbol]);
  std::string s = CSymbolNode::trainText();
  if (py.size()==1)
    return s;
  //otherwise, ambiguous, record name
  if (!m_pyParent) return ""; //output nothing! TODO could reset context for what follows - but this really shouldn't ever happen?
  
  return mgr()->m_pAlphabet->m_strConversionTrainStart + mgr()->m_vGroupNames[m_pyParent] + mgr()->m_pAlphabet->m_strConversionTrainStop + s;
}
