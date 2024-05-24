#pragma once

#include "Actions.h"
#include "AlphabetManager.h"
#include "Event.h"

namespace Dasher {

//This class does not actually manage that much. First of all, it just holds a bunch of events that listeners and handlers can subscribe to.
class CActionManager {
 public:
    void UnsubscribeAll(void* Listener)
    {
        OnCharEntered.Unsubscribe(Listener);
        OnCharRemoved.Unsubscribe(Listener);
        OnSpeak.Unsubscribe(Listener);
        OnSpeakCancel.Unsubscribe(Listener);
        OnCopy.Unsubscribe(Listener);
        OnDasherStop.Unsubscribe(Listener);
        OnDasherPause.Unsubscribe(Listener);
        OnDelete.Unsubscribe(Listener);
        OnMove.Unsubscribe(Listener);
    }

    Event<CSymbolNode*, TextCharAction*> OnCharEntered;
    Event<CSymbolNode*, TextCharUndoAction*> OnCharRemoved; //Explicitly only does one char removal
    Event<CSymbolNode*, SpeechAction*> OnSpeak;
    Event<CSymbolNode*, SpeakCancelAction*> OnSpeakCancel;
    Event<CSymbolNode*, CopyAction*> OnCopy;
    Event<CSymbolNode*, StopDasherAction*> OnDasherStop;
    Event<CSymbolNode*, PauseDasherAction*> OnDasherPause;
    Event<CSymbolNode*, DeleteAction*> OnDelete;
    Event<CSymbolNode*, MoveAction*> OnMove;
};

}
