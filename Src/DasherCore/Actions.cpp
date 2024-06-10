#include "Actions.h"

#include "ActionManager.h"
#include "AlphabetManager.h"
#include "DasherInterfaceBase.h"

using namespace Dasher;

TextAction::TextAction(ActionContext context, EditDistance dist) : context(context), m_dist(dist) {}

ContextSpeechAction::ContextSpeechAction(ActionContext c, EditDistance dist): TextAction(c, dist)
{
}

void ContextSpeechAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnContextSpeak.Broadcast(Trigger, this, InterfaceBase);
}

void FixedSpeechAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnFixedSpeak.Broadcast(Trigger, this);
}

CopyAction::CopyAction(ActionContext c, EditDistance dist): TextAction(c,dist)
{
}

void CopyAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCopy.Broadcast(Trigger, this, InterfaceBase);
}

void StopDasherAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnDasherStop.Broadcast(Trigger, this);
}

void PauseDasherAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnDasherPause.Broadcast(Trigger, this);
}

void ATSPIAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnATSPI.Broadcast(Trigger, this);
}

void SpeakCancelAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
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

void DeleteAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
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

void MoveAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnMove.Broadcast(Trigger, this);
}

void TextCharAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCharEntered.Broadcast(Trigger, this);
}

void TextCharUndoAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnCharRemoved.Broadcast(Trigger, this);
}

void KeyboardAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnKeyboard.Broadcast(Trigger, this);
}

void SocketOutputAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    Manager->OnSocketOutput.Broadcast(Trigger, this);
}

void ChangeSettingsAction::Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger)
{
    //Needs to be delayed to the end of the frame, as many parameters influence the frame rendering and should not be changed during rendering of a frame
    Manager->DelayAction([Manager, Trigger, this]()
    {
        Manager->OnSettingChange.Broadcast(Trigger, this);
    });
}

std::string TextAction::getBasedOnDistance(CDasherInterfaceBase* p_Intf, EditDistance dist) {
    strLast = p_Intf->GetTextAroundCursor(dist);
    m_iStartOffset = p_Intf->GetAllContextLenght();
    return strLast;
}

std::string TextAction::getNewContext(CDasherInterfaceBase* p_Intf) {
    strLast = p_Intf->GetContext(m_iStartOffset, p_Intf->GetAllContextLenght() - m_iStartOffset);
    m_iStartOffset = p_Intf->GetAllContextLenght();
    return strLast;
}
