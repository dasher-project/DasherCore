#include "Actions.h"
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

void SpeechAction::happen(CContNode* pNode)
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

void CopyAction::happen(CContNode* pNode)
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

void StopDasherAction::happen(CContNode* pNode)
{
    pNode->mgr()->GetDasherInterface()->Done();
    pNode->mgr()->GetDasherInterface()->GetActiveInputMethod()->pause();
}

void PauseDasherAction::happen(CContNode* pNode)
{
    pNode->mgr()->GetDasherInterface()->GetActiveInputMethod()->pause();
}

void SpeakCancelAction::happen(CContNode* pNode)
{
    pNode->mgr()->GetDasherInterface()->Speak("", true);
}

DeleteAction::DeleteAction(bool bForwards, EditDistance dist): m_bForwards(bForwards), m_dist(dist)
{
}

int DeleteAction::calculateNewOffset(CContNode* pNode, int offsetBefore)
{
    if (m_bForwards)
        return offsetBefore;

    return pNode->mgr()->GetDasherInterface()->ctrlOffsetAfterMove(offsetBefore + 1, m_bForwards, m_dist) - 1;
}

void DeleteAction::happen(CContNode* pNode)
{
    pNode->mgr()->GetDasherInterface()->ctrlDelete(m_bForwards, m_dist);
}

MoveAction::MoveAction(bool bForwards, EditDistance dist): m_bForwards(bForwards), m_dist(dist)
{
}

int MoveAction::calculateNewOffset(CContNode* pNode, int offsetBefore)
{
    return pNode->mgr()->GetDasherInterface()->ctrlOffsetAfterMove(offsetBefore + 1, m_bForwards, m_dist) - 1;
}

void MoveAction::happen(CContNode* pNode)
{
    pNode->mgr()->GetDasherInterface()->ctrlMove(m_bForwards, m_dist);
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
