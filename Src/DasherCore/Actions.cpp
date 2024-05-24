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

void SpeechAction::happen(CSymbolNode* pNode)
{
    switch (context)
    {
    case Repeat:
        m_pIntf->Speak(strLast, false);
        break;
    case NewText:
        m_pIntf->Speak(getNewContext(), false);
        break;
    case Distance:
        m_pIntf->Speak(getBasedOnDistance(m_dist), false);
    }
}

CopyAction::CopyAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist): TextAction(pIntf,c,dist)
{
}

void CopyAction::happen(CSymbolNode* pNode)
{
    switch (context)
    {
    case Repeat:
        m_pIntf->CopyToClipboard(strLast);
        break;
    case NewText:
        m_pIntf->CopyToClipboard(getNewContext());
        break;
    case Distance:
        m_pIntf->CopyToClipboard(getBasedOnDistance(m_dist));
        break;
    }
}

void StopDasherAction::happen(CSymbolNode* pNode)
{
    pNode->GetInterface()->Done();
    pNode->GetInterface()->GetActiveInputMethod()->pause();
}

void PauseDasherAction::happen(CSymbolNode* pNode)
{
    pNode->GetInterface()->GetActiveInputMethod()->pause();
}

void SpeakCancelAction::happen(CSymbolNode* pNode)
{
    pNode->GetInterface()->Speak("", true);
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

void DeleteAction::happen(CSymbolNode* pNode)
{
    pNode->GetInterface()->ctrlDelete(m_bForwards, m_dist);
}

MoveAction::MoveAction(bool bForwards, EditDistance dist): m_bForwards(bForwards), m_dist(dist)
{
}

unsigned MoveAction::calculateNewOffset(CSymbolNode* pNode, int offsetBefore)
{
    return pNode->GetInterface()->ctrlOffsetAfterMove(offsetBefore + 1, m_bForwards, m_dist) - 1;
}

void MoveAction::happen(CSymbolNode* pNode)
{
    pNode->GetInterface()->ctrlMove(m_bForwards, m_dist);
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
