// DasherNode.cpp
//
// Copyright (c) 2007 David Ward
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

#include "../Common/Common.h"

// #include "AlphabetManager.h" - doesnt seem to be required - pconlon

#include "DasherInterfaceBase.h"

using namespace Dasher;
using namespace Opts;
using namespace std;

static int iNumNodes = 0;

int Dasher::currentNumNodeObjects() {return iNumNodes;}

//TODO this used to be inline - should we make it so again?
CDasherNode::CDasherNode(int iOffset, int iColour, CDasherScreen::Label *pLabel)
: onlyChildRendered(NULL),  m_iLbnd(0), m_iHbnd(CDasherModel::NORMALIZATION), m_pParent(NULL), m_iFlags(DEFAULT_FLAGS), m_iOffset(iOffset), m_iColour(iColour), m_pLabel(pLabel) {
  iNumNodes++;
}

// TODO: put this back to being inlined
CDasherNode::~CDasherNode() {
  //  std::cout << "Deleting node: " << this << std::endl;
  // Release any storage that the node manager has allocated,
  // unreference ref counted stuff etc.
  Delete_children();

  //  std::cout << "done." << std::endl;

  iNumNodes--;
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////


// Delete nephews of the child which has the specified symbol
// TODO: Need to allow for subnode
void CDasherNode::DeleteNephews(CDasherNode *pChild) {
  DASHER_ASSERT(Children().size() > 0);

  ChildMap::iterator i;
  for(i = Children().begin(); i != Children().end(); i++) {
      if(*i != pChild) {
	(*i)->Delete_children();
    }
  }
}

void CDasherNode::Delete_children() 
{
    //TODO: REIMPLEMENT
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
