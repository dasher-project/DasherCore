#pragma once

#include "Actions.h"
#include "AlphabetManager.h"
#include "Event.h"


namespace Dasher {

//This class does not actually manage that much. First of all, it just holds a bunch of events that listeners and handlers can subscribe to.
class CActionManager {
 public:
    Event<CSymbolNode*, TextCharAction*> OnCharEntered;
    Event<CSymbolNode*, TextCharUndoAction*> OnCharRemoved;
};

}
