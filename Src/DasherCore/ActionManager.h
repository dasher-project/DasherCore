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
        OnContextSpeak.Unsubscribe(Listener);
        OnSpeakCancel.Unsubscribe(Listener);
        OnCopy.Unsubscribe(Listener);
        OnDasherStop.Unsubscribe(Listener);
        OnDasherPause.Unsubscribe(Listener);
        OnDelete.Unsubscribe(Listener);
        OnMove.Unsubscribe(Listener);
    }

    void DelayAction(std::function<void()> action)
    {
        DelayedActions.push_back(action);
    }

    void ExecuteDelayedActions()
    {
        for(auto& action : DelayedActions)
        {
            action();
        }
        DelayedActions.clear();
    }

    Event<CSymbolNode*, TextCharAction*> OnCharEntered;
    Event<CSymbolNode*, TextCharUndoAction*> OnCharRemoved; //Explicitly only does one char removal
    Event<CSymbolNode*, ContextSpeechAction*, CDasherInterfaceBase*> OnContextSpeak;
    Event<CSymbolNode*, FixedSpeechAction*> OnFixedSpeak;
    Event<CSymbolNode*, SpeakCancelAction*> OnSpeakCancel;
    Event<CSymbolNode*, KeyboardAction*> OnKeyboard;
    Event<CSymbolNode*, SocketOutputAction*> OnSocketOutput;
    Event<CSymbolNode*, ChangeSettingsAction*> OnSettingChange;
    Event<CSymbolNode*, CopyAction*, CDasherInterfaceBase*> OnCopy;
    Event<CSymbolNode*, StopDasherAction*> OnDasherStop;
    Event<CSymbolNode*, PauseDasherAction*> OnDasherPause;
    Event<CSymbolNode*, ATSPIAction*> OnATSPI;
    Event<CSymbolNode*, DeleteAction*> OnDelete;
    Event<CSymbolNode*, MoveAction*> OnMove;

private:

    // Lambda Functions that are executed after the next rendering. Mostly used for actions that are triggered during rendering.
    std::vector<std::function<void()>> DelayedActions;
};

}
