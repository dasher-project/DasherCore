#include "Actions.h"

#include "ActionManager.h"
#include "AlphabetManager.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

TextAction::TextAction(CDasherInterfaceBase *pIntf, ActionContext context, EditDistance dist) : m_pIntf(pIntf), context(context), m_dist(dist) {
  m_iStartOffset= pIntf->GetAllContextLenght();
  pIntf->m_vTextActions.insert(this);
}

TextAction::~TextAction() {
  m_pIntf->m_vTextActions.erase(this);
}

SpeechAction::SpeechAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist): TextAction(pIntf, c, dist)
{
}

void SpeechAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnSpeak.Broadcast(Trigger, this);
}

CopyAction::CopyAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist): TextAction(pIntf,c,dist)
{
}

void CopyAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCopy.Broadcast(Trigger, this);
}

void StopDasherAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnDasherStop.Broadcast(Trigger, this);
}

void PauseDasherAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnDasherPause.Broadcast(Trigger, this);
}

void SpeakCancelAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnSpeakCancel.Broadcast(Trigger, this);
}

DeleteAction::DeleteAction(bool bForwards, EditDistance dist): m_bForwards(bForwards), m_dist(dist)
{
}

unsigned DeleteAction::calculateNewOffset(CSymbolNode* pNode, int offsetBefore)
{
    if (m_bForwards)
        return offsetBefore;

    return pNode->GetInterface()->ctrlOffsetAfterMove(offsetBefore + 1, m_bForwards, m_dist) - 1;
}

void DeleteAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnDelete.Broadcast(Trigger, this);
}

MoveAction::MoveAction(bool bForwards, EditDistance dist): m_bForwards(bForwards), m_dist(dist)
{
}

unsigned MoveAction::calculateNewOffset(CSymbolNode* pNode, int offsetBefore)
{
    return pNode->GetInterface()->ctrlOffsetAfterMove(offsetBefore + 1, m_bForwards, m_dist) - 1;
}

void MoveAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnMove.Broadcast(Trigger, this);
}

void TextCharAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCharEntered.Broadcast(Trigger, this);
}

void TextCharUndoAction::Broadcast(CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCharRemoved.Broadcast(Trigger, this);
}

std::string TextAction::getBasedOnDistance(EditDistance dist) {
    strLast = m_pIntf->GetTextAroundCursor(dist);
    m_iStartOffset = m_pIntf->GetAllContextLenght();
    return strLast;
}

std::string TextAction::getNewContext() {
    strLast = m_pIntf->GetContext(m_iStartOffset, m_pIntf->GetAllContextLenght() - m_iStartOffset);
    m_iStartOffset = m_pIntf->GetAllContextLenght();
    return strLast;
}

void TextAction::NotifyOffset(int iOffset) {
  m_iStartOffset = std::min(m_pIntf->GetAllContextLenght(), m_iStartOffset);
}
