#pragma once

#include "Actions.h"
#include "DasherModule.h"

namespace Dasher
{

class CActionModule : public CDasherModule
{
public:
    explicit CActionModule(const char* szName)
        : CDasherModule(szName)
    {}

    // List of all possible Action Types to Handle
    // Could be generated implicitly but would require possibly
    // multiple parent classes (see Type Erasure Pattern), thus here kept simple
    virtual void HandleAction(StopDasherAction* Action){}
    virtual void HandleAction(DeleteAction* Action){}
    virtual void HandleAction(MoveAction* Action){}
    virtual void HandleAction(CopyAction* Action){}
    virtual void HandleAction(SpeakCancelAction* Action){}
    virtual void HandleAction(SpeechAction* Action){}
};

}
