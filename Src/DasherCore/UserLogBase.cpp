/*
 *  UserLogBase.cpp
 *  Dasher
 *
 *  Created by Alan Lawrence on 28/03/2011.
 *  Copyright 2011 Cavendish Laboratory. All rights reserved.
 *
 */

#include "UserLogBase.h"
#include "Event.h"
#include "DasherNode.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

CUserLogBase::CUserLogBase(CDasherInterfaceBase *pInterface)
: m_iNumDeleted(0), m_pInterface(pInterface) {
    m_pInterface->OnEditEvent.Subscribe(this, [this](CEditEvent::EditEventType type, const std::string&, CDasherNode* node)
    {
        if (type == CEditEvent::EDIT_OUTPUT) {
            m_vAdded.push_back(node->GetSymbolProb());
            //output
        } else if (type == CEditEvent::EDIT_DELETE) {
            //delete
            m_iNumDeleted++;
        }
    });
};

CUserLogBase::~CUserLogBase()
{
    m_pInterface->OnEditEvent.Unsubscribe(this);
}

void CUserLogBase::HandleEvent(const CEditEvent *evt) {
  
}

void CUserLogBase::FrameEnded() {
  //pass on added/deleted if any, and get ready for next frame
  if (m_iNumDeleted) {
    DeleteSymbols(m_iNumDeleted);
    m_iNumDeleted=0;
  }
  if (!m_vAdded.empty()) {
   AddSymbols(&m_vAdded);
    m_vAdded.clear();
  }
}
