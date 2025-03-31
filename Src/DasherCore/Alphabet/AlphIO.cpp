// AlphIO.cpp



#include "AlphIO.h"

#include <string>
#include <cstring>
#include <algorithm>
#include <sstream>

using namespace Dasher;

CAlphIO::CAlphIO(CMessageDisplay *pMsgs) : AbstractXMLParser(pMsgs) {
	Alphabets["Default"] = CreateDefault();
}

SGroupInfo* CAlphIO::ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet, SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors)
{
	SGroupInfo* pNewGroup = new SGroupInfo();
    pNewGroup->iNumChildNodes = 0;
	pNewGroup->strName = group_node.attribute("name").as_string("");
	pNewGroup->strLabel = group_node.attribute("label").as_string("");
    pNewGroup->colorGroup = group_node.attribute("colorInfoName").as_string("");

	pNewGroup->pNext = previous_sibling;
    pNewGroup->pChild = nullptr;

	// symbols and groups
	pNewGroup->iStart = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
	std::vector<SGroupInfo*> new_ancestors(ancestors);
	new_ancestors.push_back(pNewGroup);
	SGroupInfo* previous_subgroup_sibling = nullptr;
	for(auto node : group_node.children())
	{
		// symbol
		if(std::strcmp(node.name(),"node") == 0)
		{
			CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1); //new char
			CurrentAlphabet->m_vCharacterDoActions.resize(CurrentAlphabet->m_vCharacterDoActions.size() + 1); //new Do Actions
			CurrentAlphabet->m_vCharacterUndoActions.resize(CurrentAlphabet->m_vCharacterUndoActions.size() + 1); //new Undo Actions
			ReadCharAttributes(node, CurrentAlphabet->m_vCharacters.back(), pNewGroup, CurrentAlphabet->m_vCharacterDoActions.back(), CurrentAlphabet->m_vCharacterUndoActions.back());
			pNewGroup->iNumChildNodes++;
		}

		// group
		if(std::strcmp(node.name(),"group") == 0)
		{
			SGroupInfo* newChildGroup = ParseGroupRecursive(node, CurrentAlphabet, previous_subgroup_sibling, new_ancestors);
			if(newChildGroup == nullptr) continue;
			pNewGroup->iNumChildNodes++;
			pNewGroup->pChild = newChildGroup;
			previous_subgroup_sibling = newChildGroup;
		}
	}

	pNewGroup->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
    if (pNewGroup->iEnd == pNewGroup->iStart) {
		//empty or defunct group. Delete it now, and remove from sibling chain
		SGroupInfo* parent = (ancestors.empty() ? CurrentAlphabet : ancestors.back());
		parent->pChild = pNewGroup->pNext; //the actually previous node
		delete pNewGroup;
		return nullptr;
    }

	//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
	ReverseChildList(pNewGroup->pChild);

	return pNewGroup;
}

bool Dasher::CAlphIO::Parse(pugi::xml_document & document, const std::string, bool bUser)
{
	pugi::xml_node alphabet = document.document_element();

	if(std::strcmp(alphabet.name(), "alphabet") != 0) return false; // a non <alphabet ...> node

	CAlphInfo* CurrentAlphabet = new CAlphInfo();
	CurrentAlphabet->AlphID = alphabet.attribute("name").as_string();
	CurrentAlphabet->TrainingFile = alphabet.attribute("trainingFilename").as_string();
	CurrentAlphabet->PreferredColors = alphabet.attribute("colorsName").as_string();

	// orientation
	const std::string orientation_type = alphabet.attribute("orientation").as_string("LR");
    if (orientation_type == "RL") {
        CurrentAlphabet->Orientation = Options::RightToLeft;
    }
    else if (orientation_type == "TB") {
        CurrentAlphabet->Orientation = Options::TopToBottom;
    }
    else if (orientation_type == "BT") {
        CurrentAlphabet->Orientation = Options::BottomToTop;
    }
    else{
        CurrentAlphabet->Orientation = Options::LeftToRight;
	}

	// conversion mode
	const std::string conversion_mode = alphabet.attribute("conversionMode").as_string("none");
    if (conversion_mode == "mandarin") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::Mandarin;
    }
    else if (conversion_mode == "routingContextInsensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextInsensitive;
    }
    else if (conversion_mode == "routingContextSensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextSensitive;
    }
    else{ //none
        CurrentAlphabet->m_iConversionID = CAlphInfo::None;
	}


	// groups
	SGroupInfo* previous_sibling = nullptr;
	for(pugi::xml_node& child : alphabet.children())
	{
		if(std::strcmp(child.name(),"group") == 0){
			SGroupInfo* newGroup = ParseGroupRecursive(child, CurrentAlphabet, previous_sibling, {});
			CurrentAlphabet->iNumChildNodes++;
			CurrentAlphabet->pChild = newGroup; //always the last parsed child, as later reverse operation puts it as the first one
			previous_sibling = newGroup;
		}
	}

	CurrentAlphabet->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
	//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
	ReverseChildList(CurrentAlphabet->pChild);

	Alphabets[CurrentAlphabet->AlphID] = CurrentAlphabet;

	return true;
}

void CAlphIO::GetAlphabets(std::vector<std::string>* AlphabetList) const {
	AlphabetList->clear();

	for (auto [AlphabetID, Alphabet] : Alphabets){
		AlphabetList->push_back(Alphabet->AlphID);
	}
}

std::string CAlphIO::GetDefault() const {
	std::string DefaultExternalAlphabet = "English with limited punctuation";
	if(Alphabets.count(DefaultExternalAlphabet) != 0) {
		return DefaultExternalAlphabet;
	}

	return "Default";
}

const CAlphInfo *CAlphIO::GetInfo(const std::string &AlphabetID) const {
	auto it = Alphabets.find(AlphabetID);
	if (it == Alphabets.end()) //if we don't have the alphabet they ask for,
		it = Alphabets.find(GetDefault()); //give them default - it's better than nothing
	return it->second;
}

CAlphInfo *CAlphIO::CreateDefault() {
	// TODO I appreciate these strings should probably be in a resource file.
	// Not urgent though as this is not intended to be used. It's just a
	// last ditch effort in case file I/O totally fails.
	CAlphInfo* Default = new CAlphInfo();
	Default->AlphID = "Default";
	Default->TrainingFile = "training_english_GB.txt";
	Default->PreferredColors = "Default";
	Default->Orientation = Options::LeftToRight;
	Default->colorGroup = "lowercase";

	Default->pChild = nullptr;

	std::string Chars = "abcdefghijklmnopqrstuvwxyz";
	Default->m_vCharacters.resize(Chars.size());
	//fill in structs for characters in Chars...
	for(int i = 0; i < Chars.size(); i++) {
		Default->m_vCharacters[i].Text = Chars[i];
		Default->m_vCharacters[i].Display = Chars[i];
		Default->m_vCharacters[i].ColorGroupOffset = i;
		Default->m_vCharacters[i].parentGroup = Default;
	}

	Default->iStart=1;
    Default->iEnd= static_cast<int>(Default->m_vCharacters.size())+1;
	Default->iNumChildNodes = static_cast<int>(Default->m_vCharacters.size());
	Default->pNext = nullptr;
	Default->pChild = nullptr;

	return Default;
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::pair<bool, EditDistance> parseDistance(std::string const & value)
{
	EditDistance e = EDIT_NONE;
	if(ends_with(value, "char")) e = EDIT_CHAR;
	if(ends_with(value, "word")) e = EDIT_WORD;
	if(ends_with(value, "sentence")) e = EDIT_SENTENCE;
	if(ends_with(value, "paragraph")) e = EDIT_PARAGRAPH;
	if(ends_with(value, "all")) e = EDIT_ALL;
	return {value.rfind("next", 0) == 0, e};
}

inline std::vector<std::vector<unsigned short>> parseKeyArray(const std::string& keycodes)
{
    std::istringstream keycodeStream(keycodes);
    std::string code;
	std::vector<std::vector<unsigned short>> keyArray;
    while (std::getline(keycodeStream, code, ',')) {
		std::vector<unsigned short> keycodesArray;

		for (int i=0; i<code.length(); i+=2) {
            if(i+1 < code.length()){
                keycodesArray.push_back(static_cast<unsigned short>(std::stoul(std::string(&code[i], 2), nullptr, 16)));
            }
        }

		keyArray.push_back(keycodesArray);
    }
	return keyArray;
}

void CAlphIO::ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character& alphabet_character, SGroupInfo* parentGroup, std::vector<Action*>& DoActions, std::vector<Action*>& UndoActions) {

	if(xml_node.type() == pugi::node_null) return;

	alphabet_character.Display = xml_node.attribute("label").as_string();
	alphabet_character.Text = xml_node.attribute("text").as_string(alphabet_character.Display.c_str());

	for(auto potentialActions : xml_node.children())
	{
	    if(std::strcmp(potentialActions.name(),"textCharAction") == 0)
	    {
			DoActions.push_back(new TextCharAction());
	        UndoActions.push_back(new TextCharUndoAction());
	    }
	    else if(std::strcmp(potentialActions.name(),"deleteTextAction") == 0 && !potentialActions.attribute("distance").empty())
	    {
			const std::string distance = potentialActions.attribute("distance").as_string();
            const std::pair<bool, EditDistance> d = parseDistance(distance);
	        DoActions.push_back(new DeleteAction(d.first, d.second));
	    }
		else if(std::strcmp(potentialActions.name(),"moveCursorAction") == 0 && !potentialActions.attribute("distance").empty())
	    {
			const std::string distance = potentialActions.attribute("distance").as_string();
            const std::pair<bool, EditDistance> d = parseDistance(distance);
	        DoActions.push_back(new MoveAction(d.first, d.second));
	    }
		else if(std::strcmp(potentialActions.name(),"fixedTTSAction") == 0 && !potentialActions.attribute("text").empty())
	    {
	        DoActions.push_back(new FixedSpeechAction(potentialActions.attribute("text").as_string()));
	    }
		else if(std::strcmp(potentialActions.name(),"moveCursorAction") == 0 && !potentialActions.attribute("distance").empty())
	    {
			const std::string distance = potentialActions.attribute("context").as_string();
            const std::pair<bool, EditDistance> d = parseDistance(distance);
	        DoActions.push_back(new ContextSpeechAction(TextAction::Distance, d.second));
	    }
		else if(std::strcmp(potentialActions.name(),"repeatTTSAction") == 0)
	    {
	        DoActions.push_back(new ContextSpeechAction(TextAction::Repeat));
	    }
	    else if(std::strcmp(potentialActions.name(),"contextTTSAction") == 0)
	    {
			const std::string context = potentialActions.attribute("context").as_string();
            const std::pair<bool, EditDistance> c = parseDistance(context);
	        DoActions.push_back(new ContextSpeechAction(TextAction::Distance, c.second));
	    }
		else if(std::strcmp(potentialActions.name(),"stopTTSAction") == 0)
	    {
	        DoActions.push_back(new SpeakCancelAction());
	    }
		else if(std::strcmp(potentialActions.name(),"pauseDasherAction") == 0)
	    {
	        DoActions.push_back(new PauseDasherAction(static_cast<long>(potentialActions.attribute("time").as_llong(-1))));
	    }
		else if(std::strcmp(potentialActions.name(),"atspiAction") == 0)
	    {
	        if(!potentialActions.attribute("action").empty()) DoActions.push_back(new ATSPIAction(potentialActions.attribute("action").as_string()));
	        if(!potentialActions.attribute("undoAction").empty()) UndoActions.push_back(new ATSPIAction(potentialActions.attribute("undoAction ").as_string()));
	    }
	    else if(std::strcmp(potentialActions.name(),"keyboardAction") == 0
			&& (!potentialActions.attribute("key").empty()
			|| !potentialActions.attribute("press").empty()
			|| !potentialActions.attribute("release").empty()
			))
	    {
			std::string keycodes;
			KeyboardAction::pressType p;

			if(!potentialActions.attribute("key").empty()) keycodes = potentialActions.attribute("key").as_string(); p = KeyboardAction::KEY_PRESS_RELEASE;
			if(!potentialActions.attribute("press").empty()) keycodes = potentialActions.attribute("press").as_string(); p = KeyboardAction::KEY_PRESS;
			if(!potentialActions.attribute("release").empty()) keycodes = potentialActions.attribute("release").as_string(); p = KeyboardAction::KEY_RELEASE;
	        DoActions.push_back(new KeyboardAction(p, parseKeyArray(keycodes)));

			if(!potentialActions.attribute("undoKey").empty()) keycodes = potentialActions.attribute("undoKey").as_string(); p = KeyboardAction::KEY_PRESS_RELEASE;
			if(!potentialActions.attribute("undoPress").empty()) keycodes = potentialActions.attribute("undoPress").as_string(); p = KeyboardAction::KEY_PRESS;
			if(!potentialActions.attribute("undoRelease").empty()) keycodes = potentialActions.attribute("undoRelease").as_string(); p = KeyboardAction::KEY_RELEASE;
	        UndoActions.push_back(new KeyboardAction(p, parseKeyArray(keycodes)));
	    }
	    else if(std::strcmp(potentialActions.name(),"socketOutputAction") == 0)
	    {
			const bool suppressNewLine = potentialActions.attribute("suppressNewline").as_bool(false);
	        if(!potentialActions.attribute("doString").empty()) DoActions.push_back(new SocketOutputAction(potentialActions.attribute("socketName").as_string(""), potentialActions.attribute("doString").as_string(""), suppressNewLine));
	        if(!potentialActions.attribute("undoString").empty()) UndoActions.push_back(new SocketOutputAction(potentialActions.attribute("socketName").as_string(""), potentialActions.attribute("undoString").as_string(""), suppressNewLine));
	    }
		else if(std::strcmp(potentialActions.name(),"changeSettingAction") == 0 && !potentialActions.attribute("settingsName").empty())
	    {
			const std::pair<Parameter, Settings::ParameterType> param =  Settings::GetParameter(potentialActions.attribute("settingsName").as_string());
			if(!potentialActions.attribute("doValue").empty()){
			    if (param.second == Settings::PARAM_STRING) DoActions.push_back(new ChangeSettingsAction(param.first, std::string(potentialActions.attribute("doValue").as_string())));
			    if (param.second == Settings::PARAM_BOOL) DoActions.push_back(new ChangeSettingsAction(param.first, potentialActions.attribute("doValue").as_bool()));
			    if (param.second == Settings::PARAM_LONG) DoActions.push_back(new ChangeSettingsAction(param.first, static_cast<long>(potentialActions.attribute("doValue").as_llong())));
			}
	        if(!potentialActions.attribute("undoValue").empty())
	        {
	            if (param.second == Settings::PARAM_STRING) UndoActions.push_back(new ChangeSettingsAction(param.first, std::string(potentialActions.attribute("undoValue").as_string())));
			    if (param.second == Settings::PARAM_BOOL) UndoActions.push_back(new ChangeSettingsAction(param.first, potentialActions.attribute("undoValue").as_bool()));
			    if (param.second == Settings::PARAM_LONG) UndoActions.push_back(new ChangeSettingsAction(param.first, static_cast<long>(potentialActions.attribute("undoValue").as_llong())));
	        }
	    }
	}

	alphabet_character.parentGroup = parentGroup;
	alphabet_character.ColorGroupOffset = parentGroup->iNumChildNodes;
	alphabet_character.fixedProbability = xml_node.attribute("fixedProbability").as_float(-1);
	alphabet_character.speedFactor = xml_node.attribute("speedFactor").as_float(-1);
}

// Reverses the internal linked list for the given SGroupInfo
// input given GroupInfo eg. pointer to G_1 with [G_1->G_2->G_3->G_4->Null]
// result [G_4->G_3->G_2->G_1->Null]
void CAlphIO::ReverseChildList(SGroupInfo *&pList) {
	SGroupInfo *pFirst = pList;
	SGroupInfo *pPrev = NULL;
	while (pFirst) {
		SGroupInfo *pNext = pFirst->pNext;
		pFirst->pNext = pPrev;
		pPrev = pFirst;
		pFirst = pNext;
	}
	pList=pPrev;
}

CAlphIO::~CAlphIO() {
	for (auto [AlphabetID, Alphabet] : Alphabets) {
		delete Alphabet;
	}
}
