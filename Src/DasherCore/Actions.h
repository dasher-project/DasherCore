#pragma once
#include <string>
#include <vector>

namespace Dasher {
    class CSymbolNode;
    class CActionManager;
    class CDasherInterfaceBase;

typedef enum EditDistance : unsigned int {
    EDIT_CHAR, EDIT_WORD, EDIT_SENTENCE, EDIT_PARAGRAPH, EDIT_FILE, EDIT_LINE, EDIT_PAGE, EDIT_SELECTION, EDIT_ALL, EDIT_NONE
} EditDistance;

class Action {
public:
    Action() = default;
    virtual ~Action() = default;

    virtual unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) { return offsetBefore; }
    virtual void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) = 0;
};

// Baseclass for actions that use some context of the already entered text
class TextAction : public Action {
    public:
		typedef enum ActionContext
	    {
	        Repeat,
		    NewText,
		    Distance
	    } ActionContext;

        TextAction(ActionContext context, EditDistance dist);
        std::string getBasedOnDistance(CDasherInterfaceBase* p_Intf, EditDistance dist);
        std::string getNewContext(CDasherInterfaceBase* p_Intf);

        int m_iStartOffset = 0;
        std::string strLast;
	    ActionContext context;
	    EditDistance m_dist;
};

class ContextSpeechAction : public TextAction
{
public:
    ContextSpeechAction(ActionContext c, EditDistance dist = EDIT_NONE);

    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class FixedSpeechAction : public Action
{
public:
    FixedSpeechAction(std::string text) : text(text){};

    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;

    std::string text;
};

class CopyAction : public TextAction
{
public:
    CopyAction(ActionContext c, EditDistance dist = EDIT_NONE);
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class StopDasherAction : public Action
{
public:
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class PauseDasherAction : public Action
{
public:
    PauseDasherAction(long timeInMs) : time(timeInMs) {}
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
    long time = - 1;
};

class ATSPIAction : public Action
{
public:
    ATSPIAction(const std::string& action) : action(action) {}
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
    std::string action;
};

class SpeakCancelAction : public Action
{
public:
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class DeleteAction : public Action
{
public:
	DeleteAction(bool bForwards, EditDistance dist);

	unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) override;

    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;

	bool m_bForwards;
	EditDistance m_dist;
};

class MoveAction : public Action
{
public:
	MoveAction(bool bForwards, EditDistance dist);

	unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) override;

    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;

	bool m_bForwards;
	EditDistance m_dist;
};

class TextCharAction : public Action
{
public:
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class TextCharUndoAction : public TextCharAction
{
public:
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
};

class KeyboardAction : public Action
{
public:
    enum pressType
    {
        KEY_PRESS, KEY_RELEASE, KEY_PRESS_RELEASE
    };

    KeyboardAction(pressType type, std::vector<std::vector<unsigned short>> keycodes) : type(type), keycodes(keycodes) {}
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;

    pressType type;
    std::vector<std::vector<unsigned short>> keycodes;
};

class SocketOutputAction : public Action
{
public:
    SocketOutputAction(const std::string& socketName, const std::string& action) : action(action), socketName(socketName) {}
    void Broadcast(CDasherInterfaceBase* InterfaceBase, CActionManager* Manager, CSymbolNode* Trigger) override;
    std::string socketName;
    std::string action;
};

}

