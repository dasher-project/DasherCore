
//
// Copyright (c) 2007 David Ward


// #include "AlphabetManager.h" - doesnt seem to be required - pconlon

#include "DasherInterfaceBase.h"

using namespace Dasher;

static int iNumNodes = 0;

int Dasher::currentNumNodeObjects() {return iNumNodes;}

//TODO this used to be inline - should we make it so again?
CDasherNode::CDasherNode(int iOffset, CDasherScreen::Label *pLabel)
: onlyChildRendered(NULL),  m_iLbnd(0), m_iHbnd(CDasherModel::NORMALIZATION), m_pParent(NULL), m_iFlags(DEFAULT_FLAGS), m_iOffset(iOffset), m_pLabel(pLabel) {
  iNumNodes++;
}

// TODO: put this back to being inlined
CDasherNode::~CDasherNode() {
  //  std::cout << "Deleting node: " << this << std::endl;
  // Release any storage that the node manager has allocated,
  // unreference ref counted stuff etc.
  DeleteChildren();

  //  std::cout << "done." << std::endl;

  iNumNodes--;
}

/////////////////////////////////////////////////////////////////////////////
// Functions implemented from description:
//CDasherNode::GetContext(CDasherInterfaceBase *pInterface, const CAlphabetMap *pAlphabet, vector<symbol> &vContextSymbols, int iOffset, int iLength)
//- Check if the Flag NF_SEEN was set
//    - Set:
//        Get the string-context from the CDasherInterfaceBase
//        Fill vContextSymbols from the given alphabet
//    - Not set:
//        Check if parent is actually set
//        Get context from parent
void CDasherNode::GetContext(CDasherInterfaceBase* pInterface, const CAlphabetMap* pAlphabetMap, std::vector<symbol>& vContextSymbols, int iOffset, int iLength){
    if (GetFlag(NF_SEEN)) {
        std::string context =  pInterface->GetContext(iOffset, iLength);
        pAlphabetMap->GetSymbols(vContextSymbols, context);
    }
    else {
        if (m_pParent != nullptr) {
            m_pParent->GetContext(pInterface, pAlphabetMap, vContextSymbols, iOffset, iLength);
        }
    }
}
//CDasherNode::SetFlag(int, bool)
//- Depending on the bool, either set or remove the flag from the m_iFlags

void CDasherNode::SetFlag(int iFlag, bool bValue) {
    if (bValue) {
        m_iFlags |= iFlag;
    }
    else {
        m_iFlags &= ~iFlag;
    }
}
//
//CDasherNode::OrphanChild(class Dasher::CDasherNode*)
//- Iterate over all children of this node
//   - Do the following operation for all children except the given one :
//      - Delete their Children
//      - Delete / free the child
//- Clear our children list(do not call delete on the given child)
//- Orphan the given child by setting the parent pointer to nullptr
//- Reset the NF_ALLCHILDREN flag on the called node

void CDasherNode::OrphanChild(CDasherNode* pChild) {
    for (CDasherNode* child : Children()) {  //Children is an std::deque
        if (child != pChild) {
            child->DeleteChildren();
            delete(child);
        }
    }
    Children().clear();
    pChild->m_pParent = nullptr;
    SetFlag(NF_ALLCHILDREN, false);
}


//CDasherNode::Reparent(class Dasher::CDasherNode*, unsigned int, unsigned int)
//  - Check and error if the following conditions are met :
//      -given node(new parent) is not defined
//      - current parent of this node is defined
//      - new parent has not set the flag NF_ALLCHILDREN
//      - the given iLbnd is equal to the the last childs iHbnd of the new parent(or equal to zero if the new parent does not have children)
//  - Reparent this node to the new parent
//      - Set the parent pointer to the new parent
//      - Register this node in the parents children list
//      - Set the given iLbnd and iHbnd values for this node
// Asserts skipped since Dasher_Assert is defined as ((void)true)
void CDasherNode::Reparent(CDasherNode* pNewParent, unsigned int iLower, unsigned int iUpper) {
    m_pParent = pNewParent;
    m_pParent->Children().push_back(this);
    m_iLbnd = iLower;
    m_iHbnd = iUpper;
}

// Delete nephews of the child which has the specified symbol
// TODO: Need to allow for subnode
void CDasherNode::DeleteNephews(CDasherNode *pChild) {
  DASHER_ASSERT(Children().size() > 0);

  ChildMap::iterator i;
  for(i = Children().begin(); i != Children().end(); i++) {
      if(*i != pChild) {
	(*i)->DeleteChildren();
    }
  }
}
//CDasherNode::DeleteChildren()
//- Call delete on all children
//- Clear Child List
//- Reset the NF_ALLCHILDREN flag
//- Set the OnlyChildRendered to nullptr
void CDasherNode::DeleteChildren(){
    for (CDasherNode* child : Children()) {
        delete(child);
    }
    Children().clear();
    SetFlag(NF_ALLCHILDREN, false);
    onlyChildRendered = nullptr;
}

int CDasherNode::MostProbableChild() {
  int iMax(0);
  int iCurrent;

  for(ChildMap::iterator it(m_mChildren.begin()); it != m_mChildren.end(); ++it) {
    iCurrent = (*it)->Range();

    if(iCurrent > iMax)
      iMax = iCurrent;
  }

  return iMax;
}

bool CDasherNode::GameSearchChildren(symbol sym) {
  for (ChildMap::iterator i = Children().begin(); i != Children().end(); i++) {
    if ((*i)->GameSearchNode(sym)) return true;
  }
  return false;
}
