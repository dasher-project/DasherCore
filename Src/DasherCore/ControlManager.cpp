// ControlManager.cpp
//
// Copyright (c) 2007 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "ControlManager.h"
#include "DasherInterfaceBase.h"
#include <cstring>

using namespace Dasher;

CControlBase::CControlBase(CSettingsUser* pCreateFrom, CDasherInterfaceBase* pInterface, CNodeCreationManager* pNCManager)
	: CSettingsUser(pCreateFrom), m_pInterface(pInterface), m_pNCManager(pNCManager), m_pScreen(NULL), m_pRoot(NULL)
{
}

NodeTemplate* CControlBase::GetRootTemplate()
{
	return m_pRoot;
}

void CControlBase::SetRootTemplate(NodeTemplate* pRoot)
{
	if (m_pRoot || !pRoot) throw "SetRoot should only be called once, with a non-null root";
	m_pRoot = pRoot;
}

int CControlBase::getColour(NodeTemplate* pTemplate, CDasherNode* pParent)
{
	if (pTemplate->m_iColour != -1) return pTemplate->m_iColour;
	if (pParent) return (pParent->ChildCount() % 99) + 11;
	return 11;
}

CDasherNode* CControlBase::GetRoot(CDasherNode* pContext, int iOffset)
{
	if (!m_pRoot) return m_pNCManager->GetAlphabetManager()->GetRoot(pContext, false, iOffset);

	CContNode* pNewNode = new CContNode(iOffset, m_pRoot, this);

	// FIXME - handle context properly

	//  pNewNode->SetContext(m_pLanguageModel->CreateEmptyContext());

	return pNewNode;
}

void CControlBase::ChangeScreen(CDasherScreen* pScreen)
{
	if (m_pScreen == pScreen) return;
	m_pScreen = pScreen;
	std::deque<NodeTemplate*> templateQueue(1, m_pRoot);
	std::set<NodeTemplate*> allTemplates(templateQueue.begin(), templateQueue.end());
	while (!templateQueue.empty())
	{
		NodeTemplate* head = templateQueue.front();
		templateQueue.pop_front();
		delete head->m_pLabel;
		head->m_pLabel = pScreen->MakeLabel(head->m_strLabel);
		for (auto child : head->successors)
		{
			if (!child) continue; //an escape back to the alphabet, no label/successors here
			if (allTemplates.find(child) == allTemplates.end())
			{
				allTemplates.insert(child);
				templateQueue.push_back(child);
			}
		}
	}
}

NodeTemplate::NodeTemplate(const std::string& strLabel, int iColour)
	: m_strLabel(strLabel), m_iColour(iColour), m_pLabel(NULL)
{
}

NodeTemplate::~NodeTemplate()
{
	delete m_pLabel;
}

CContNode::CContNode(int iOffset, NodeTemplate* pTemplate, CControlBase* pMgr)
	: CDasherNode(iOffset, pTemplate->m_pLabel), m_pTemplate(pTemplate), m_pMgr(pMgr)
{
}

double CContNode::SpeedMul()
{
	return m_pMgr->GetBoolParameter(BP_SLOW_CONTROL_BOX) ? 0.5 : 1;
}

void CContNode::PopulateChildren()
{
	CDasherNode* pNewNode;

	const unsigned int iNChildren(static_cast<unsigned int>(m_pTemplate->successors.size()));
	unsigned int iLbnd(0), iIdx(0);
	int newOffset = m_pTemplate->calculateNewOffset(this, offset());

	for (auto child : m_pTemplate->successors)
	{
		const unsigned int iHbnd((++iIdx * CDasherModel::NORMALIZATION) / iNChildren);

		if (child == NULL)
		{
			// Escape back to alphabet

			pNewNode = m_pMgr->m_pNCManager->GetAlphabetManager()->GetRoot(this, false, newOffset + 1);
		}
		else
		{
			pNewNode = new CContNode(newOffset, child, m_pMgr);
		}
		pNewNode->Reparent(this, iLbnd, iHbnd);
		iLbnd = iHbnd;
		DASHER_ASSERT(GetChildren().back()==pNewNode);
	}
}

int CContNode::ExpectedNumChildren()
{
	return static_cast<int>(m_pTemplate->successors.size());
}

void CContNode::Output()
{
	m_pTemplate->happen(this);
}

const ColorPalette::Color& CContNode::getLabelColor(const ColorPalette* colorPalette)
{
    //TODO: I dont know know these work. This needs to be implemented at a later stage
    return ColorPalette::noColor;
}

const ColorPalette::Color& CContNode::getOutlineColor(const ColorPalette* colorPalette)
{
    //TODO: I dont know know these work. This needs to be implemented at a later stage
    return ColorPalette::noColor;
}

const ColorPalette::Color& CContNode::getNodeColor(const ColorPalette* colorPalette)
{
    //TODO: I dont know know these work. This needs to be implemented at a later stage
	// Probably depends on getColour(m_pRoot, pContext)
    return ColorPalette::noColor;
}

const std::list<NodeTemplate*>& CControlParser::parsedNodes()
{
	return m_vParsed;
}

///Template used for all node defns read in from XML - just
/// execute a list of Actions.
class XMLNodeTemplate : public NodeTemplate
{
public:
	XMLNodeTemplate(const std::string& label, int color) : NodeTemplate(label, color)
	{
	}

	int calculateNewOffset(CContNode* pNode, int offsetBefore) override
	{
		int newOffset = offsetBefore;
		for (auto pAction : actions)
			newOffset = pAction->calculateNewOffset(pNode, newOffset);
		return newOffset;
	}

	void happen(CContNode* pNode) override
	{
		for (auto pAction : actions)
			pAction->happen(pNode);
	}

	~XMLNodeTemplate()
	{
		for (auto pAction : actions)
			delete pAction;
	}

	std::vector<Action*> actions;
};

CControlParser::CControlParser(CMessageDisplay* pMsgs) : AbstractXMLParser(pMsgs), m_bUser(false)
{
}

void CControlParser::ParseNodeRecursive(pugi::xml_node node, std::list<NodeTemplate*>& parent)
{
	XMLNodeTemplate* newNode = new XMLNodeTemplate(node.attribute("label").as_string(), node.attribute("color").as_int(-1));
	parent.push_back(newNode);

	std::string nodeName = node.attribute("name").as_string();
	if (!nodeName.empty()) namedNodes[nodeName] = newNode; //all refs resolved at end.

	for(pugi::xml_node sub_node : node.children())
	{
		std::string tag_name = sub_node.name();
		if(tag_name == "node")
		{
			ParseNodeRecursive(sub_node, newNode->successors);
		}
		else if (tag_name == "ref")
		{
			std::string target = sub_node.attribute("name").as_string();
			if (auto it = namedNodes.find(target); it != namedNodes.end())
			{
				parent.push_back(it->second);
			}
			else
			{
				parent.push_back(nullptr);
				unresolvedRefs.push_back({&parent.back(), target});
			}
		}
		else if (tag_name == "alph")
		{
			parent.push_back(nullptr);
		}
		else if (NodeTemplate* n = parseOther(sub_node))
		{
			parent.push_back(n);
		}
		else if (Action* a = parseAction(sub_node))
		{
			DASHER_ASSERT(!nodeStack.empty());
			newNode->actions.push_back(a);
		}
	}
}


bool CControlParser::Parse(pugi::xml_document& document, const std::string, bool bUser)
{
	if (m_bUser)
	{
		// User File Mode!
		if (!bUser) return true; // So ignore system!
	}
	else
	{
		// System File Mode
		if (bUser) m_vParsed.clear(); // If we were user mode before, clear everything that was parsed so far
		m_bUser = true;
	}

	namedNodes.clear();
	unresolvedRefs.clear();

	for(pugi::xml_node sub_node : document.children())
	{
		std::string tag_name = sub_node.name();
		if(tag_name == "node")
		{
			ParseNodeRecursive(sub_node, m_vParsed);
		}
		else if (tag_name == "alph")
		{
			m_vParsed.push_back(nullptr);
		}
	}

	//resolve any forward references to nodes declared later
	for (auto [list_space, searched_ID] : unresolvedRefs)
	{
		auto target = namedNodes.find(searched_ID);
		if (target != namedNodes.end()){
			*(list_space) = target->second;
		}
	}

	//somehow, need to clear out any refs that weren't resolved...???
	return true;
}

CControlManager::CControlManager(CSettingsUser* pCreateFrom, CNodeCreationManager* pNCManager, CDasherInterfaceBase* pInterface)
	: CSettingsObserver(pCreateFrom), CControlBase(pCreateFrom, pInterface, pNCManager), CControlParser(pInterface)
{
	//TODO, used to be able to change label+colour of root/pause/stop from controllabels.xml
	// (or, get the root node title "control" from the alphabet!)
	SetRootTemplate(new NodeTemplate("", 8)); //default NodeTemplate does nothing

	// Key in actions map is name plus arguments in alphabetical order.
	m_actions["stop"] = new Stop();
	m_actions["pause"] = new Pause();
	if (pInterface->SupportsSpeech())
	{
		m_actions["speak what=all"] = new SpeechAction(pInterface, TextAction::Distance, EDIT_FILE);
		m_actions["speak what=page"] = new SpeechAction(pInterface, TextAction::Distance,  EDIT_PAGE);
		m_actions["speak what=paragraph"] = new SpeechAction(pInterface, TextAction::Distance, EDIT_PARAGRAPH);
		m_actions["speak what=sentence"] = new SpeechAction(pInterface, TextAction::Distance, EDIT_SENTENCE);
		m_actions["speak what=line"] = new SpeechAction(pInterface, TextAction::Distance,  EDIT_LINE);
		m_actions["speak what=word"] = new SpeechAction(pInterface, TextAction::Distance, EDIT_WORD);
		m_actions["speak what=new"] = new SpeechAction(pInterface, TextAction::NewText);
		m_actions["speak what=repeat"] = new SpeechAction(pInterface, TextAction::Repeat);
		m_actions["speak what=cancel"] = new SpeakCancel();
	}
	if (pInterface->SupportsClipboard())
	{
		m_actions["copy what=all"] = new CopyAction(pInterface, TextAction::Distance, EDIT_FILE);
		m_actions["copy what=page"] = new CopyAction(pInterface, TextAction::Distance,  EDIT_PAGE);
		m_actions["copy what=paragraph"] = new CopyAction(pInterface, TextAction::Distance, EDIT_PARAGRAPH);
		m_actions["copy what=sentence"] = new CopyAction(pInterface, TextAction::Distance, EDIT_SENTENCE);
		m_actions["copy what=line"] = new CopyAction(pInterface, TextAction::Distance, EDIT_LINE);
		m_actions["copy what=word"] = new CopyAction(pInterface, TextAction::Distance, EDIT_WORD);
		m_actions["copy what=new"] = new CopyAction(pInterface, TextAction::NewText);
		m_actions["copy what=repeat"] = new CopyAction(pInterface, TextAction::Repeat);
	}
	m_actions["move dist=char forward=yes"] = new Move(true, EDIT_CHAR);
	m_actions["move dist=word forward=yes"] = new Move(true, EDIT_WORD);
	m_actions["move dist=line forward=yes"] = new Move(true, EDIT_LINE);
	m_actions["move dist=sentence forward=yes"] = new Move(true, EDIT_SENTENCE);
	m_actions["move dist=paragraph forward=yes"] = new Move(true, EDIT_PARAGRAPH);
	m_actions["move dist=page forward=yes"] = new Move(true, EDIT_PAGE);
	m_actions["move dist=all forward=yes"] = new Move(true, EDIT_FILE);

	m_actions["move dist=char forward=no"] = new Move(false, EDIT_CHAR);
	m_actions["move dist=word forward=no"] = new Move(false, EDIT_WORD);
	m_actions["move dist=line forward=no"] = new Move(false, EDIT_LINE);
	m_actions["move dist=sentence forward=no"] = new Move(false, EDIT_SENTENCE);
	m_actions["move dist=paragraph forward=no"] = new Move(false, EDIT_PARAGRAPH);
	m_actions["move dist=page forward=no"] = new Move(false, EDIT_PAGE);
	m_actions["move dist=all forward=no"] = new Move(false, EDIT_FILE);

	m_actions["delete dist=char forward=yes"] = new Delete(true, EDIT_CHAR);
	m_actions["delete dist=word forward=yes"] = new Delete(true, EDIT_WORD);
	m_actions["delete dist=line forward=yes"] = new Delete(true, EDIT_LINE);
	m_actions["delete dist=sentence forward=yes"] = new Delete(true, EDIT_SENTENCE);
	m_actions["delete dist=paragraph forward=yes"] = new Delete(true, EDIT_PARAGRAPH);
	m_actions["delete dist=page forward=yes"] = new Delete(true, EDIT_PAGE);
	m_actions["delete dist=all forward=yes"] = new Delete(true, EDIT_FILE);

	m_actions["delete dist=char forward=no"] = new Delete(false, EDIT_CHAR);
	m_actions["delete dist=word forward=no"] = new Delete(false, EDIT_WORD);
	m_actions["delete dist=line forward=no"] = new Delete(false, EDIT_LINE);
	m_actions["delete dist=sentence forward=no"] = new Delete(false, EDIT_SENTENCE);
	m_actions["delete dist=paragraph forward=no"] = new Delete(false, EDIT_PARAGRAPH);
	m_actions["delete dist=page forward=no"] = new Delete(false, EDIT_PAGE);
	m_actions["delete dist=all forward=no"] = new Delete(false, EDIT_FILE);
}

NodeTemplate* CControlManager::parseOther(pugi::xml_node node)
{
	if (strcmp(node.name(), "root") == 0) return GetRootTemplate();
	return CControlParser::parseOther(node);
}

Action* CControlManager::parseAction(pugi::xml_node node)
{
	std::map<std::string, std::string> arguments;
	for(auto attribute : node.attributes())
	{
		arguments[attribute.name()] = attribute.as_string();
	}

	std::stringstream key;
	// Key in actions map is name plus arguments in alphabetical order.
	key << node.name();
	if (!arguments.empty())
	{
		for (auto arg : arguments)
		{
			key << " " << arg.first << "=" << arg.second;
		}
	}

	if (auto it = m_actions.find(key.str()); it != m_actions.end()){
		return it->second;
	}

	return CControlParser::parseAction(node);
}

CControlManager::~CControlManager()
{
}

void CControlManager::HandleEvent(Parameter parameter)
{
	switch (parameter)
	{
	case BP_COPY_ALL_ON_STOP:
	case BP_SPEAK_ALL_ON_STOP:
	case SP_INPUT_FILTER:
		updateActions();
	}
}

void CControlManager::updateActions()
{
	// decide if removal of pause and stop are worth the trouble 
	// reimplement if yes 
	// imo with control.xml it isn't.
	GetRootTemplate()->successors.clear();

	for (auto pNode : parsedNodes())
		GetRootTemplate()->successors.push_back(pNode);

	// If nothing was read from control.xml, add alphabet and control box
	if (GetRootTemplate()->successors.empty())
	{
		m_pMsgs->Message("Control box is empty.", false);
		GetRootTemplate()->successors.push_back(NULL);
		GetRootTemplate()->successors.push_back(GetRootTemplate());
	}

	if (CDasherScreen* pScreen = m_pScreen)
	{
		//hack to make ChangeScreen do something
		m_pScreen = NULL; //i.e. make it think the screen has changed
		ChangeScreen(pScreen);
	}
}

CControlBoxIO::CControlBoxIO(CMessageDisplay* pMsgs) : AbstractXMLParser(pMsgs)
{
}

CControlManager* CControlBoxIO::CreateControlManager(
	const std::string& id, CSettingsUser* pCreateFrom, CNodeCreationManager* pNCManager,
	CDasherInterfaceBase* pInterface) const
{
	auto mgr = new CControlManager(pCreateFrom, pNCManager, pInterface);
	auto it = m_controlFiles.find(id);
	if (it != m_controlFiles.end())
		mgr->ParseFile(it->second, true);
	mgr->updateActions();
	return mgr;
}


void CControlBoxIO::GetControlBoxes(std::vector<std::string>* pList) const
{
	for (auto id_filename : m_controlFiles)
		pList->push_back(id_filename.first);
}

bool CControlBoxIO::Parse(pugi::xml_document& document, const std::string, bool bUser)
{
	std::string name = document.child("control").attribute("name").as_string();

	if (!bUser && m_controlFiles.count(name)) return true; // Ignore system files if that name already taken

	m_controlFiles[name] = GetDesc();

	return true;
}
