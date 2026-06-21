// AlphIO.cpp
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

#include "AlphIO.h"
#include "DasherCore/ControlManager.h"

#include <string>
#include <cstring>
#include <algorithm>
#include <sstream>

using namespace Dasher;

CAlphIO::CAlphIO(CMessageDisplay* pMsgs) : AbstractXMLParser(pMsgs) {
    Alphabets["Default"] = CreateDefault();
}

SGroupInfo* CAlphIO::ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet,
                                         SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors) {
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
    for (auto node : group_node.children()) {
        // symbol (v6 "node" or v5 "s")
        if (std::strcmp(node.name(), "node") == 0 || std::strcmp(node.name(), "s") == 0) {
            CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1); // new char
            CurrentAlphabet->m_vCharacterDoActions.resize(CurrentAlphabet->m_vCharacterDoActions.size() +
                                                          1); // new Do Actions
            CurrentAlphabet->m_vCharacterUndoActions.resize(CurrentAlphabet->m_vCharacterUndoActions.size() +
                                                            1); // new Undo Actions
            ReadCharAttributes(node, CurrentAlphabet->m_vCharacters.back(), pNewGroup,
                               CurrentAlphabet->m_vCharacterDoActions.back(),
                               CurrentAlphabet->m_vCharacterUndoActions.back());
            pNewGroup->iNumChildNodes++;
        }

        // group
        if (std::strcmp(node.name(), "group") == 0) {
            SGroupInfo* newChildGroup =
                ParseGroupRecursive(node, CurrentAlphabet, previous_subgroup_sibling, new_ancestors);
            if (newChildGroup == nullptr) continue;
            pNewGroup->iNumChildNodes++;
            pNewGroup->pChild = newChildGroup;
            previous_subgroup_sibling = newChildGroup;
        }
    }

    pNewGroup->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
    if (pNewGroup->iEnd == pNewGroup->iStart) {
        // empty or defunct group. Delete it now, and remove from sibling chain
        SGroupInfo* parent = (ancestors.empty() ? CurrentAlphabet : ancestors.back());
        parent->pChild = pNewGroup->pNext; // the actually previous node
        delete pNewGroup;
        return nullptr;
    }

    // child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
    ReverseChildList(pNewGroup->pChild);

    return pNewGroup;
}

bool Dasher::CAlphIO::Parse(pugi::xml_document& document, const std::string strDesc, bool bUser) {
    pugi::xml_node root = document.document_element();

    // v5 format: <alphabets> root containing <alphabet> children
    if (std::strcmp(root.name(), "alphabets") == 0) {
        bool any = false;
        for (pugi::xml_node alpha : root.children("alphabet")) {
            if (ParseSingle(alpha, strDesc, bUser)) any = true;
        }
        return any;
    }

    // v6 format: <alphabet> root (single alphabet per file)
    if (std::strcmp(root.name(), "alphabet") != 0) return false;
    return ParseSingle(root, strDesc, bUser);
}

bool Dasher::CAlphIO::ParseSingle(pugi::xml_node alphabet, const std::string, bool bUser) {
    CAlphInfo* CurrentAlphabet = new CAlphInfo();
    CurrentAlphabet->AlphID = alphabet.attribute("name").as_string();
    CurrentAlphabet->TrainingFile = alphabet.attribute("trainingFilename").as_string();
    CurrentAlphabet->PreferredColors = alphabet.attribute("colorsName").as_string();

    // Handle v5 metadata child elements (v6 uses attributes on <alphabet>)
    for (pugi::xml_node meta : alphabet.children()) {
        const char* name = meta.name();
        if (std::strcmp(name, "train") == 0 && CurrentAlphabet->TrainingFile.empty())
            CurrentAlphabet->TrainingFile = meta.text().as_string();
        else if (std::strcmp(name, "palette") == 0 && CurrentAlphabet->PreferredColors.empty())
            CurrentAlphabet->PreferredColors = meta.text().as_string();
        else if (std::strcmp(name, "orientation") == 0) {
            std::string otype = meta.attribute("type").as_string("LR");
            if (otype == "RL") CurrentAlphabet->Orientation = Options::RightToLeft;
            else if (otype == "TB") CurrentAlphabet->Orientation = Options::TopToBottom;
            else if (otype == "BT") CurrentAlphabet->Orientation = Options::BottomToTop;
            else CurrentAlphabet->Orientation = Options::LeftToRight;
        }
    }

    // If orientation wasn't set from v5 element, try v6 attribute
    if (CurrentAlphabet->Orientation == Options::LeftToRight) {
        const std::string orientation_type = alphabet.attribute("orientation").as_string("LR");
        if (orientation_type == "RL") {
            CurrentAlphabet->Orientation = Options::RightToLeft;
        } else if (orientation_type == "TB") {
            CurrentAlphabet->Orientation = Options::TopToBottom;
        } else if (orientation_type == "BT") {
            CurrentAlphabet->Orientation = Options::BottomToTop;
        }
    }

    // conversion mode
    const std::string conversion_mode = alphabet.attribute("conversionMode").as_string("none");
    if (conversion_mode == "mandarin") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::Mandarin;
    } else if (conversion_mode == "routingContextInsensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextInsensitive;
    } else if (conversion_mode == "routingContextSensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextSensitive;
    } else { // none
        CurrentAlphabet->m_iConversionID = CAlphInfo::None;
    }

    // groups
    SGroupInfo* previous_sibling = nullptr;
    for (pugi::xml_node& child : alphabet.children()) {
        if (std::strcmp(child.name(), "group") == 0) {
            SGroupInfo* newGroup = ParseGroupRecursive(child, CurrentAlphabet, previous_sibling, {});
            CurrentAlphabet->iNumChildNodes++;
            CurrentAlphabet->pChild =
                newGroup; // always the last parsed child, as later reverse operation puts it as the first one
            previous_sibling = newGroup;
        }
    }

    CurrentAlphabet->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
    // child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
    ReverseChildList(CurrentAlphabet->pChild);

    auto it = Alphabets.find(CurrentAlphabet->AlphID);
    if (it != Alphabets.end()) delete it->second;
    Alphabets[CurrentAlphabet->AlphID] = CurrentAlphabet;

    return true;
}

void CAlphIO::GetAlphabets(std::vector<std::string>* AlphabetList) const {
    AlphabetList->clear();

    for (const auto& [AlphabetID, Alphabet] : Alphabets) {
        AlphabetList->push_back(Alphabet->AlphID);
    }
}

std::string CAlphIO::GetDefault() const {
    std::string DefaultExternalAlphabet = "English with limited punctuation";
    if (Alphabets.count(DefaultExternalAlphabet) != 0) {
        return DefaultExternalAlphabet;
    }

    return "Default";
}

const CAlphInfo* CAlphIO::GetInfo(const std::string& AlphabetID) const {
    auto it = Alphabets.find(AlphabetID);
    if (it == Alphabets.end())             // if we don't have the alphabet they ask for,
        it = Alphabets.find(GetDefault()); // give them default - it's better than nothing
    return it->second;
}

CAlphInfo* CAlphIO::CreateDefault() {
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
    Default->m_vCharacterDoActions.resize(Chars.size());
    Default->m_vCharacterUndoActions.resize(Chars.size());
    for (size_t i = 0; i < Chars.size(); i++) {
        Default->m_vCharacters[i].Text = Chars[i];
        Default->m_vCharacters[i].Display = Chars[i];
        Default->m_vCharacters[i].ColorGroupOffset = i;
        Default->m_vCharacters[i].parentGroup = Default;
    }

    Default->iStart = 1;
    Default->iEnd = static_cast<int>(Default->m_vCharacters.size()) + 1;
    Default->iNumChildNodes = static_cast<int>(Default->m_vCharacters.size());
    Default->pNext = nullptr;
    Default->pChild = nullptr;

    return Default;
}

inline bool ends_with(std::string const& value, std::string const& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::pair<bool, EditDistance> parseDistance(std::string const& value) {
    EditDistance e = EDIT_NONE;
    if (ends_with(value, "char")) e = EDIT_CHAR;
    if (ends_with(value, "word")) e = EDIT_WORD;
    if (ends_with(value, "sentence")) e = EDIT_SENTENCE;
    if (ends_with(value, "paragraph")) e = EDIT_PARAGRAPH;
    if (ends_with(value, "all")) e = EDIT_ALL;
    return {value.rfind("next", 0) == 0, e};
}

inline std::vector<std::vector<unsigned short>> parseKeyArray(const std::string& keycodes) {
    std::istringstream keycodeStream(keycodes);
    std::string code;
    std::vector<std::vector<unsigned short>> keyArray;
    while (std::getline(keycodeStream, code, ',')) {
        std::vector<unsigned short> keycodesArray;

        for (size_t i = 0; i < code.length(); i += 2) {
            if (i + 1 < code.length()) {
                keycodesArray.push_back(static_cast<unsigned short>(std::stoul(std::string(&code[i], 2), nullptr, 16)));
            }
        }

        keyArray.push_back(keycodesArray);
    }
    return keyArray;
}

void CAlphIO::ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character& alphabet_character,
                                 SGroupInfo* parentGroup, std::vector<ControlAction*>& DoActions,
                                 std::vector<ControlAction*>& UndoActions) {

    if (xml_node.type() == pugi::node_null) return;

    // v6 uses "label"/"text" attributes; v5 uses "d"/"t" attributes
    alphabet_character.Display = xml_node.attribute("label").as_string();
    if (alphabet_character.Display.empty())
        alphabet_character.Display = xml_node.attribute("d").as_string();
    alphabet_character.Text = xml_node.attribute("text").as_string();
    if (alphabet_character.Text.empty())
        alphabet_character.Text = xml_node.attribute("t").as_string(alphabet_character.Display.c_str());

    for (auto potentialActions : xml_node.children()) {
        const char* actionName = potentialActions.name();

        if (std::strcmp(actionName, "textCharAction") == 0) {
            DoActions.push_back(new TextOutputAction(alphabet_character.Text));
            UndoActions.push_back(new TextDeleteAction(alphabet_character.Text));
            if (xml_node.attribute("text").empty() && !potentialActions.attribute("unicode").empty()) {
                int codepoint = potentialActions.attribute("unicode").as_int(-1);
                if (codepoint > 0) {
                    std::string utf8;
                    if (codepoint < 0x80) {
                        utf8 += static_cast<char>(codepoint);
                    } else if (codepoint < 0x800) {
                        utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
                        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else if (codepoint < 0x10000) {
                        utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
                        utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
                    } else {
                        utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
                        utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                        utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    alphabet_character.Text = utf8;
                    // Re-create actions with the resolved text
                    delete DoActions.back();
                    DoActions.back() = new TextOutputAction(alphabet_character.Text);
                    delete UndoActions.back();
                    UndoActions.back() = new TextDeleteAction(alphabet_character.Text);
                }
            }
        } else if (std::strcmp(actionName, "deleteTextAction") == 0 &&
                   !potentialActions.attribute("distance").empty()) {
            const std::string distance = potentialActions.attribute("distance").as_string();
            const std::pair<bool, EditDistance> d = parseDistance(distance);
            DoActions.push_back(new DeleteAction(d.first, d.second));
        } else if (std::strcmp(actionName, "moveCursorAction") == 0 &&
                   !potentialActions.attribute("distance").empty()) {
            const std::string distance = potentialActions.attribute("distance").as_string();
            const std::pair<bool, EditDistance> d = parseDistance(distance);
            DoActions.push_back(new MoveAction(d.first, d.second));
        } else if (std::strcmp(actionName, "fixedTTSAction") == 0 && !potentialActions.attribute("text").empty()) {
            DoActions.push_back(new FixedSpeakAction(potentialActions.attribute("text").as_string()));
        } else if (std::strcmp(actionName, "contextTTSAction") == 0) {
            if (potentialActions.attribute("context").empty()) {
                DoActions.push_back(new SpeakAction(TextActionBase::Repeat, EDIT_NONE));
            } else {
                const std::string context = potentialActions.attribute("context").as_string();
                const std::pair<bool, EditDistance> c = parseDistance(context);
                DoActions.push_back(new SpeakAction(TextActionBase::Distance, c.second));
            }
        } else if (std::strcmp(actionName, "stopTTSAction") == 0) {
            DoActions.push_back(new SpeakCancelAction());
        } else if (std::strcmp(actionName, "copyToClipboardAction") == 0) {
            const std::string context = potentialActions.attribute("context").as_string("all");
            if (context == "new")
                DoActions.push_back(new CopyAction(TextActionBase::NewText, EDIT_NONE));
            else {
                const std::pair<bool, EditDistance> c = parseDistance(context);
                DoActions.push_back(new CopyAction(TextActionBase::Distance, c.second));
            }
        } else if (std::strcmp(actionName, "stopDasherAction") == 0) {
            DoActions.push_back(new StopAction());
        } else if (std::strcmp(actionName, "pauseDasherAction") == 0) {
            DoActions.push_back(new PauseAction());
        } else if (std::strcmp(actionName, "atspiAction") == 0) {
            if (!potentialActions.attribute("action").empty())
                DoActions.push_back(new ATSPIAction(potentialActions.attribute("action").as_string()));
            if (!potentialActions.attribute("undoAction").empty())
                UndoActions.push_back(new ATSPIAction(potentialActions.attribute("undoAction").as_string()));
        } else if (std::strcmp(actionName, "keyboardAction") == 0 &&
                   (!potentialActions.attribute("key").empty() || !potentialActions.attribute("press").empty() ||
                    !potentialActions.attribute("release").empty())) {
            std::string keycodes;
            KeyboardAction::PressType p;

            if (!potentialActions.attribute("key").empty()) keycodes = potentialActions.attribute("key").as_string();
            if (!potentialActions.attribute("press").empty())
                keycodes = potentialActions.attribute("press").as_string();
            if (!potentialActions.attribute("release").empty())
                keycodes = potentialActions.attribute("release").as_string();
            p = KeyboardAction::KEY_RELEASE;
            DoActions.push_back(new KeyboardAction(p, parseKeyArray(keycodes)));

            if (!potentialActions.attribute("undoKey").empty())
                keycodes = potentialActions.attribute("undoKey").as_string();
            if (!potentialActions.attribute("undoPress").empty())
                keycodes = potentialActions.attribute("undoPress").as_string();
            if (!potentialActions.attribute("undoRelease").empty())
                keycodes = potentialActions.attribute("undoRelease").as_string();
            p = KeyboardAction::KEY_RELEASE;
            UndoActions.push_back(new KeyboardAction(p, parseKeyArray(keycodes)));
        } else if (std::strcmp(actionName, "socketOutputAction") == 0) {
            const bool suppressNewLine = potentialActions.attribute("suppressNewline").as_bool(false);
            if (!potentialActions.attribute("doString").empty())
                DoActions.push_back(new SocketOutputAction(potentialActions.attribute("socketName").as_string(""),
                                                           potentialActions.attribute("doString").as_string(""),
                                                           suppressNewLine));
            if (!potentialActions.attribute("undoString").empty())
                UndoActions.push_back(new SocketOutputAction(potentialActions.attribute("socketName").as_string(""),
                                                             potentialActions.attribute("undoString").as_string(""),
                                                             suppressNewLine));
        } else if (std::strcmp(actionName, "changeSettingAction") == 0 &&
                   !potentialActions.attribute("settingsName").empty()) {
            const std::pair<Parameter, Settings::ParameterType> param =
                Settings::GetParameter(potentialActions.attribute("settingsName").as_string());
            if (!potentialActions.attribute("doValue").empty()) {
                if (param.second == Settings::PARAM_STRING)
                    DoActions.push_back(new ChangeSettingAction(
                        param.first, std::string(potentialActions.attribute("doValue").as_string())));
                if (param.second == Settings::PARAM_BOOL)
                    DoActions.push_back(
                        new ChangeSettingAction(param.first, potentialActions.attribute("doValue").as_bool()));
                if (param.second == Settings::PARAM_LONG)
                    DoActions.push_back(new ChangeSettingAction(
                        param.first, static_cast<long>(potentialActions.attribute("doValue").as_llong())));
            }
            if (!potentialActions.attribute("undoValue").empty()) {
                if (param.second == Settings::PARAM_STRING)
                    UndoActions.push_back(new ChangeSettingAction(
                        param.first, std::string(potentialActions.attribute("undoValue").as_string())));
                if (param.second == Settings::PARAM_BOOL)
                    UndoActions.push_back(
                        new ChangeSettingAction(param.first, potentialActions.attribute("undoValue").as_bool()));
                if (param.second == Settings::PARAM_LONG)
                    UndoActions.push_back(new ChangeSettingAction(
                        param.first, static_cast<long>(potentialActions.attribute("undoValue").as_llong())));
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
void CAlphIO::ReverseChildList(SGroupInfo*& pList) {
    SGroupInfo* pFirst = pList;
    SGroupInfo* pPrev = NULL;
    while (pFirst) {
        SGroupInfo* pNext = pFirst->pNext;
        pFirst->pNext = pPrev;
        pPrev = pFirst;
        pFirst = pNext;
    }
    pList = pPrev;
}

CAlphIO::~CAlphIO() {
    for (const auto& [AlphabetID, Alphabet] : Alphabets) {
        delete Alphabet;
    }
}
