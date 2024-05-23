#pragma once
#include <string>

namespace Dasher {
class CDasherInterfaceBase;
class CControlManager;
class CControlBase;
class CContNode;

typedef enum EditDistance : unsigned int {
    EDIT_CHAR, EDIT_WORD, EDIT_SENTENCE, EDIT_PARAGRAPH, EDIT_FILE, EDIT_LINE, EDIT_PAGE, EDIT_SELECTION, EDIT_NONE
} EditDistance;

class Action {
public:
    Action() = default;
    virtual ~Action() = default;

    virtual int calculateNewOffset(CContNode *pNode, int offsetBefore) { return offsetBefore; }
    virtual void happen(CContNode *pNode) {}
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

        void happen(CContNode* pNode) override = 0;

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

	void happen(CContNode* pNode) override;
};

class CopyAction : public TextAction
{
public:
	CopyAction(CDasherInterfaceBase* pIntf, ActionContext c, EditDistance dist = EDIT_NONE);

	void happen(CContNode* pNode) override;
};

class StopDasherAction : public Action
{
public:
	void happen(CContNode* pNode);
};

class PauseDasherAction : public Action
{
public:
	void happen(CContNode* pNode);
};

class SpeakCancelAction : public Action
{
public:
	void happen(CContNode* pNode);
};

class DeleteAction : public Action
{
	const bool m_bForwards;
	const EditDistance m_dist;
public:
	DeleteAction(bool bForwards, EditDistance dist);

	int calculateNewOffset(CContNode* pNode, int offsetBefore) override;

    void happen(CContNode* pNode) override;
};

class MoveAction : public Action
{
	const bool m_bForwards;
	const EditDistance m_dist;
public:
	MoveAction(bool bForwards, EditDistance dist);

	int calculateNewOffset(CContNode* pNode, int offsetBefore) override;

    void happen(CContNode* pNode) override;
};
}
