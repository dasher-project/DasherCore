#pragma once
#include <string>

namespace Dasher
{
    class CActionManager;
}

namespace Dasher {
    class CSymbolNode;
    class CDasherNode;
    class CDasherInterfaceBase;
    class CControlBase;
    class CContNode;

typedef enum EditDistance : unsigned int {
    EDIT_CHAR, EDIT_WORD, EDIT_SENTENCE, EDIT_PARAGRAPH, EDIT_FILE, EDIT_LINE, EDIT_PAGE, EDIT_SELECTION, EDIT_NONE
} EditDistance;

class Action {
public:
    Action() = default;
    virtual ~Action() = default;

    virtual unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) { return offsetBefore; }
    virtual void happen(CSymbolNode* pNode) {}
    virtual void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) = 0;
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

        TextAction(CDasherInterfaceBase *pMgr, ActionContext context, EditDistance dist);
        std::string getBasedOnDistance(EditDistance dist);
        std::string getNewContext();
        void NotifyOffset(int iOffset);
        ~TextAction() override;

        void happen(CSymbolNode* pNode) override = 0;

    protected:
        CDasherInterfaceBase *m_pIntf;
        int m_iStartOffset;
        std::string strLast;
	    ActionContext context;
	    EditDistance m_dist;
};

class SpeechAction : public TextAction
{
public:
    SpeechAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist = EDIT_NONE);

	void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class CopyAction : public TextAction
{
public:
	CopyAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist = EDIT_NONE);

	void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class StopDasherAction : public Action
{
public:
	void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class PauseDasherAction : public Action
{
public:
	void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class SpeakCancelAction : public Action
{
public:
	void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class DeleteAction : public Action
{
	const bool m_bForwards;
	const EditDistance m_dist;
public:
	DeleteAction(bool bForwards, EditDistance dist);

	unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) override;

    void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class MoveAction : public Action
{
	const bool m_bForwards;
	const EditDistance m_dist;
public:
	MoveAction(bool bForwards, EditDistance dist);

	unsigned calculateNewOffset(CSymbolNode* pNode, int offsetBefore) override;

    void happen(CSymbolNode* pNode) override;
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override {}
};

class TextCharAction : public Action
{
public:
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override;
};

class TextCharUndoAction : public TextCharAction
{
public:
    void Broadcast(CActionManager* Manager, CSymbolNode* Trigger) override;
};

}

