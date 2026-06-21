// ControlManager.cpp
//
// Implementation of the unified action system and control node tree.
//
// Copyright (c) 2007-2024 The Dasher Team
//
// This file is part of Dasher. Dasher is free software; you can redistribute
// it and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.

#include "ControlManager.h"

#include "DasherInterfaceBase.h"
#include "DasherModel.h"
#include "InputFilter.h"
#include "NodeCreationManager.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <functional>
#include <set>

using namespace Dasher;

// ── ControlAction base ────────────────────────────────────────────────────

int ControlAction::calculateNewOffset(CDasherInterfaceBase* intf, int offsetBefore) {
    return offsetBefore;
}

// ── CustomAction ──────────────────────────────────────────────────────────

void CustomAction::execute(CDasherInterfaceBase* intf) {
    if (m_callback) m_callback(m_name, m_attrs);
}

// ── ActionRegistry ─────────────────────────────────────────────────────────

void ActionRegistry::registerFactory(const std::string& name, ActionFactory factory) {
    m_factories[name] = {std::move(factory), nullptr};
}

void ActionRegistry::registerCustomAction(const std::string& name, CustomActionCallback callback) {
    m_factories[name] = {nullptr, std::move(callback)};
}

ControlAction* ActionRegistry::create(const std::string& name, const std::map<std::string, std::string>& attrs) const {
    auto it = m_factories.find(name);
    if (it == m_factories.end()) return nullptr;

    if (it->second.customCallback) return new CustomAction(name, attrs, it->second.customCallback);

    return it->second.factory(attrs);
}

bool ActionRegistry::hasAction(const std::string& name) const {
    return m_factories.find(name) != m_factories.end();
}

// ── NodeTemplate ───────────────────────────────────────────────────────────

int NodeTemplate::calculateNewOffset(CDasherInterfaceBase* intf, int offsetBefore) const {
    int newOffset = offsetBefore;
    for (auto* action : m_actions)
        newOffset = action->calculateNewOffset(intf, newOffset);
    return newOffset;
}

void NodeTemplate::executeActions(CDasherInterfaceBase* intf) const {
    for (auto* action : m_actions)
        action->execute(intf);
}

// ── CContNode ──────────────────────────────────────────────────────────────

CContNode::CContNode(int iOffset, int iColour, NodeTemplate* pTemplate, CControlManager* pMgr)
    : CDasherNode(iOffset, pTemplate->m_pLabel), m_pTemplate(pTemplate), m_pMgr(pMgr), m_iColourIndex(iColour) {}

CNodeManager* CContNode::mgr() const {
    return m_pMgr;
}

double CContNode::SpeedMul() {
    return m_pMgr->GetSettingsStore()->GetBoolParameter(BP_SLOW_CONTROL_BOX) ? 0.5 : 1.0;
}

void CContNode::PopulateChildren() {
    const unsigned int iNChildren = static_cast<unsigned int>(m_pTemplate->successors.size());
    if (iNChildren == 0) return;

    unsigned int iLbnd = 0, iIdx = 0;
    int newOffset = m_pTemplate->calculateNewOffset(m_pMgr->GetInterface(), offset());

    for (auto* child : m_pTemplate->successors) {
        const unsigned int iHbnd = (++iIdx * CDasherModel::NORMALIZATION) / iNChildren;

        CDasherNode* pNewNode;
        if (child == nullptr) {
            // <alph/> escape — bridge back to alphabet
            pNewNode = m_pMgr->GetNCManager()->GetAlphabetManager()->GetRoot(this, false, newOffset + 1);
        } else {
            pNewNode = new CContNode(newOffset, m_pMgr->getColour(child, this), child, m_pMgr);
        }
        pNewNode->Reparent(this, iLbnd, iHbnd);
        iLbnd = iHbnd;
    }
}

int CContNode::ExpectedNumChildren() {
    return static_cast<int>(m_pTemplate->successors.size());
}

void CContNode::Do() {
    m_pTemplate->executeActions(m_pMgr->GetInterface());
}

const ColorPalette::Color& CContNode::getNodeColor(const ColorPalette*) {
    return CControlManager::indexToColor(m_iColourIndex);
}

// ── Colour mapping ─────────────────────────────────────────────────────────

const ColorPalette::Color& CControlManager::indexToColor(int iIndex) {
    // Semantic colors matching the default control.xml
    static const ColorPalette::Color controlGrey(128, 128, 128);
    static const ColorPalette::Color controlGreen(60, 180, 75);
    static const ColorPalette::Color controlAmber(220, 160, 0);
    static const ColorPalette::Color controlRed(200, 50, 50);

    switch (iIndex) {
    case 8:
        return controlGrey;
    case 240:
        return controlGreen;
    case 241:
        return controlAmber;
    case 242:
        return controlRed;
    default:
        break;
    }

    // For custom control XML with arbitrary color indices (e.g. v5 imports),
    // cycle through a palette of distinct, readable colors. This approximates
    // v5's behavior of looking up palette indices.
    static const ColorPalette::Color cyclePalette[] = {
        ColorPalette::Color(60, 140, 200),  // blue
        ColorPalette::Color(80, 170, 100),  // green
        ColorPalette::Color(200, 120, 50),  // orange
        ColorPalette::Color(160, 80, 180),  // purple
        ColorPalette::Color(50, 160, 180),  // teal
        ColorPalette::Color(220, 140, 60),  // amber
        ColorPalette::Color(180, 60, 100),  // pink
        ColorPalette::Color(100, 100, 110), // grey
        ColorPalette::Color(90, 130, 50),   // olive
        ColorPalette::Color(140, 90, 200),  // violet
        ColorPalette::Color(200, 80, 80),   // coral
        ColorPalette::Color(50, 130, 210),  // sky
    };
    return cyclePalette[iIndex % 12];
}

// ── Built-in action implementations ────────────────────────────────────────

void StopAction::execute(CDasherInterfaceBase* intf) {
    intf->Done();
    intf->GetActiveInputMethod()->pause();
}

void PauseAction::execute(CDasherInterfaceBase* intf) {
    intf->GetActiveInputMethod()->pause();
}

void MoveAction::execute(CDasherInterfaceBase* intf) {
    intf->ctrlMove(m_bForwards, m_dist);
}

int MoveAction::calculateNewOffset(CDasherInterfaceBase* intf, int offsetBefore) {
    return static_cast<int>(
               intf->ctrlOffsetAfterMove(static_cast<unsigned int>(offsetBefore + 1), m_bForwards, m_dist)) -
           1;
}

void DeleteAction::execute(CDasherInterfaceBase* intf) {
    intf->ctrlDelete(m_bForwards, m_dist);
}

int DeleteAction::calculateNewOffset(CDasherInterfaceBase* intf, int offsetBefore) {
    if (m_bForwards) return offsetBefore;
    return static_cast<int>(
               intf->ctrlOffsetAfterMove(static_cast<unsigned int>(offsetBefore + 1), m_bForwards, m_dist)) -
           1;
}

std::string TextActionBase::getText(CDasherInterfaceBase* intf) {
    switch (m_context) {
    case Repeat:
        return m_strLast;
    case NewText: {
        int currentLen = intf->GetAllContextLenght();
        int start = m_iStartOffset;
        m_iStartOffset = currentLen;
        if (currentLen <= start) return {};
        return intf->GetContext(static_cast<unsigned int>(start), static_cast<unsigned int>(currentLen - start));
    }
    case Distance:
        m_strLast = intf->GetTextAroundCursor(m_dist);
        return m_strLast;
    default:
        return {};
    }
}

void SpeakAction::execute(CDasherInterfaceBase* intf) {
    std::string text = getText(intf);
    if (!text.empty()) intf->Speak(text, false);
}

void SpeakCancelAction::execute(CDasherInterfaceBase* intf) {
    intf->Speak("", true);
}

void CopyAction::execute(CDasherInterfaceBase* intf) {
    std::string text = getText(intf);
    if (!text.empty()) intf->CopyToClipboard(text);
}

void TextOutputAction::execute(CDasherInterfaceBase* intf) {
    intf->editOutput(m_text, nullptr);
}

void TextDeleteAction::execute(CDasherInterfaceBase* intf) {
    intf->editDelete(m_text, nullptr);
}

void FixedSpeakAction::execute(CDasherInterfaceBase* intf) {
    if (!m_text.empty()) intf->Speak(m_text, false);
}

void ChangeSettingAction::execute(CDasherInterfaceBase* intf) {
    // Defer to end of frame — parameter changes during rendering are unsafe
    intf->DelayAction([intf, param = m_parameter, val = m_newValue]() {
        auto* settings = intf->GetSettingsStore();
        if (std::holds_alternative<bool>(val))
            settings->SetBoolParameter(param, std::get<bool>(val));
        else if (std::holds_alternative<long>(val))
            settings->SetLongParameter(param, std::get<long>(val));
        else if (std::holds_alternative<std::string>(val))
            settings->SetStringParameter(param, std::get<std::string>(val));
    });
}

void KeyboardAction::execute(CDasherInterfaceBase* intf) {
    // Default: no-op. Platforms override via interface virtual methods if needed.
}

void SocketOutputAction::execute(CDasherInterfaceBase* intf) {
    // Default: no-op. Platforms override via interface virtual methods if needed.
}

void ATSPIAction::execute(CDasherInterfaceBase* intf) {
    // Default: no-op. Platforms override via interface virtual methods if needed.
}

// ── CControlManager ────────────────────────────────────────────────────────

CControlManager::CControlManager(CSettingsStore* pSettingsStore, CDasherInterfaceBase* pInterface,
                                 CNodeCreationManager* pNCManager, CMessageDisplay* pMsgs)
    : AbstractXMLParser(pMsgs), m_pSettingsStore(pSettingsStore), m_pInterface(pInterface), m_pNCManager(pNCManager) {
    m_pRoot = new NodeTemplate("", 8);
    registerBuiltinActions();
}

CControlManager::~CControlManager() {
    // Collect all unique templates for deletion (graph may have cycles via <ref>)
    std::set<NodeTemplate*> allTemplates;
    if (m_pRoot) {
        std::deque<NodeTemplate*> queue;
        allTemplates.insert(m_pRoot);
        queue.push_back(m_pRoot);
        while (!queue.empty()) {
            NodeTemplate* head = queue.front();
            queue.pop_front();
            for (auto* child : head->successors) {
                if (child && allTemplates.find(child) == allTemplates.end()) {
                    allTemplates.insert(child);
                    queue.push_back(child);
                }
            }
        }
    }
    for (auto* tmpl : allTemplates)
        delete tmpl;
}

void CControlManager::registerBuiltinActions() {
    // Simple actions (no parameters)
    m_registry.registerFactory("stop", [](const auto&) { return new StopAction(); });
    m_registry.registerFactory("pause", [](const auto&) { return new PauseAction(); });
    m_registry.registerFactory("speak_cancel", [](const auto&) { return new SpeakCancelAction(); });

    // Move/delete with dist + forward attributes
    auto distFactory = [](auto createDist) -> ActionFactory {
        return [createDist](const std::map<std::string, std::string>& attrs) -> ControlAction* {
            auto it = attrs.find("dist");
            if (it == attrs.end()) {
                it = attrs.find("distance");
                if (it == attrs.end()) return nullptr;
            }

            // Parse "dist" value. Old control.xml uses: char, word, sentence, paragraph, page, all
            // Alphabet XML uses: nextChar, previousChar, nextWord, etc.
            std::string distStr = it->second;
            bool forwards = true;
            EditDistance dist = EDIT_CHAR;

            // Check for "next"/"previous" prefix (alphabet XML style)
            if (distStr.rfind("next", 0) == 0) {
                forwards = true;
                distStr = distStr.substr(4); // remove "next"
            } else if (distStr.rfind("previous", 0) == 0) {
                forwards = false;
                distStr = distStr.substr(8); // remove "previous"
            } else {
                // control.xml style: check explicit forward attribute
                auto fwdIt = attrs.find("forward");
                if (fwdIt != attrs.end()) forwards = (fwdIt->second == "yes" || fwdIt->second == "true");
            }

            // Parse distance type
            if (distStr == "Char" || distStr == "char")
                dist = EDIT_CHAR;
            else if (distStr == "Word" || distStr == "word")
                dist = EDIT_WORD;
            else if (distStr == "Line" || distStr == "line")
                dist = EDIT_LINE;
            else if (distStr == "Sent." || distStr == "Sentence" || distStr == "sentence")
                dist = EDIT_SENTENCE;
            else if (distStr == "Para." || distStr == "Paragraph" || distStr == "paragraph")
                dist = EDIT_PARAGRAPH;
            else if (distStr == "Page" || distStr == "page")
                dist = EDIT_PAGE;
            else if (distStr == "All" || distStr == "all")
                dist = EDIT_FILE;
            else
                return nullptr;

            return createDist(forwards, dist);
        };
    };

    m_registry.registerFactory(
        "move", distFactory([](bool f, EditDistance d) -> ControlAction* { return new MoveAction(f, d); }));

    m_registry.registerFactory(
        "delete", distFactory([](bool f, EditDistance d) -> ControlAction* { return new DeleteAction(f, d); }));

    // Speak/copy with "what" attribute
    auto textActionFactory = [](char mode) -> ActionFactory {
        // mode: 's' = speak, 'c' = copy
        return [mode](const std::map<std::string, std::string>& attrs) -> ControlAction* {
            auto it = attrs.find("what");
            if (it == attrs.end()) it = attrs.find("context");

            if (it == attrs.end()) return nullptr;

            std::string what = it->second;

            if (what == "cancel") return mode == 's' ? static_cast<ControlAction*>(new SpeakCancelAction()) : nullptr;

            if (what == "new")
                return mode == 's' ? static_cast<ControlAction*>(new SpeakAction(TextActionBase::NewText, EDIT_NONE))
                                   : static_cast<ControlAction*>(new CopyAction(TextActionBase::NewText, EDIT_NONE));

            if (what == "repeat")
                return mode == 's' ? static_cast<ControlAction*>(new SpeakAction(TextActionBase::Repeat, EDIT_NONE))
                                   : static_cast<ControlAction*>(new CopyAction(TextActionBase::Repeat, EDIT_NONE));

            // Distance-based: all, page, paragraph, sentence, line, word
            EditDistance dist = EDIT_FILE;
            if (what == "all")
                dist = EDIT_FILE;
            else if (what == "page")
                dist = EDIT_PAGE;
            else if (what == "paragraph")
                dist = EDIT_PARAGRAPH;
            else if (what == "sentence")
                dist = EDIT_SENTENCE;
            else if (what == "line")
                dist = EDIT_LINE;
            else if (what == "word")
                dist = EDIT_WORD;
            else
                return nullptr;

            return mode == 's' ? static_cast<ControlAction*>(new SpeakAction(TextActionBase::Distance, dist))
                               : static_cast<ControlAction*>(new CopyAction(TextActionBase::Distance, dist));
        };
    };

    m_registry.registerFactory("speak", textActionFactory('s'));
    m_registry.registerFactory("copy", textActionFactory('c'));

    // Alphabet XML action names (for backward compat with existing alphabets)
    m_registry.registerFactory("textCharAction", [](const auto& attrs) -> ControlAction* {
        // TextCharAction outputs the symbol text. The text is set later
        // by AlphIO when it knows the symbol's output text.
        return nullptr; // handled specially in AlphIO
    });

    m_registry.registerFactory("stopDasherAction", [](const auto&) -> ControlAction* { return new StopAction(); });
    m_registry.registerFactory("pauseDasherAction", [](const auto&) -> ControlAction* { return new PauseAction(); });
    m_registry.registerFactory("stopTTSAction", [](const auto&) -> ControlAction* { return new SpeakCancelAction(); });
    m_registry.registerFactory("fixedTTSAction", [](const auto& attrs) -> ControlAction* {
        auto it = attrs.find("text");
        if (it == attrs.end()) return nullptr;
        return new FixedSpeakAction(it->second);
    });
}

CDasherNode* CControlManager::GetRoot(CDasherNode* pContext, int iOffset) {
    if (!m_pRoot) return nullptr;
    return new CContNode(iOffset, getColour(m_pRoot, pContext), m_pRoot, this);
}

void CControlManager::ChangeScreen(CDasherScreen* pScreen) {
    if (m_pScreen == pScreen) return;
    m_pScreen = pScreen;

    // Walk the template graph and create/update labels
    std::deque<NodeTemplate*> templateQueue;
    if (m_pRoot) templateQueue.push_back(m_pRoot);
    std::set<NodeTemplate*> allTemplates(templateQueue.begin(), templateQueue.end());

    while (!templateQueue.empty()) {
        NodeTemplate* head = templateQueue.front();
        templateQueue.pop_front();
        delete head->m_pLabel;
        head->m_pLabel = pScreen->MakeLabel(head->m_strLabel);
        for (auto* child : head->successors) {
            if (!child) continue;
            if (allTemplates.find(child) == allTemplates.end()) {
                allTemplates.insert(child);
                templateQueue.push_back(child);
            }
        }
    }
}

int CControlManager::getColour(NodeTemplate* pTemplate, CDasherNode* pParent) {
    if (pTemplate->m_iColour != -1) return pTemplate->m_iColour;
    if (pParent) return (pParent->ChildCount() % 99) + 11;
    return 11;
}

void CControlManager::updateActions() {
    m_pRoot->successors.clear();
    for (auto* pNode : m_parsedNodes)
        m_pRoot->successors.push_back(pNode);

    if (m_pRoot->successors.empty()) {
        m_pMsgs->Message("Control box is empty.", false);
        m_pRoot->successors.push_back(nullptr); // <alph/>
        m_pRoot->successors.push_back(m_pRoot); // loop back
    }

    if (m_pScreen) {
        CDasherScreen* scr = m_pScreen;
        m_pScreen = nullptr;
        ChangeScreen(scr);
    }
}

// ── XML parsing ────────────────────────────────────────────────────────────

bool CControlManager::Parse(pugi::xml_document& document, const std::string filePath, bool bUser) {
    m_parsedNodes.clear();

    std::map<std::string, NodeTemplate*> namedNodes;
    std::vector<std::pair<std::list<NodeTemplate*>::iterator, std::string>> unresolvedRefs;
    std::vector<NodeTemplate*> nodeStack;

    std::function<void(pugi::xml_node&, std::list<NodeTemplate*>&)> processElement =
        [&](pugi::xml_node& xmlNode, std::list<NodeTemplate*>& parentSuccessors) {
            std::string name = xmlNode.name();

            if (name == "node") {
                std::string label, nodeName;
                int color = -1;
                for (pugi::xml_attribute attr : xmlNode.attributes()) {
                    std::string attrName = attr.name();
                    if (attrName == "name")
                        nodeName = attr.value();
                    else if (attrName == "label")
                        label = attr.value();
                    else if (attrName == "color")
                        color = std::atoi(attr.value());
                }

                auto* n = new NodeTemplate(label, color);
                parentSuccessors.push_back(n);
                nodeStack.push_back(n);
                if (!nodeName.empty()) namedNodes[nodeName] = n;

                for (pugi::xml_node child : xmlNode.children())
                    processElement(child, n->successors);

                nodeStack.pop_back();
            } else if (name == "ref") {
                std::string target;
                for (pugi::xml_attribute attr : xmlNode.attributes()) {
                    if (std::string(attr.name()) == "name") target = attr.value();
                }
                auto it = namedNodes.find(target);
                if (it != namedNodes.end()) {
                    parentSuccessors.push_back(it->second);
                } else {
                    parentSuccessors.push_back(nullptr);
                    unresolvedRefs.emplace_back(std::prev(parentSuccessors.end()), target);
                }
            } else if (name == "alph") {
                parentSuccessors.push_back(nullptr); // <alph/> escape
            } else if (name == "root") {
                parentSuccessors.push_back(m_pRoot); // loop back to root
            } else if (m_registry.hasAction(name)) {
                // Action element — add to current node's action list
                if (!nodeStack.empty()) {
                    std::map<std::string, std::string> attrs;
                    for (pugi::xml_attribute attr : xmlNode.attributes())
                        attrs[attr.name()] = attr.value();

                    ControlAction* action = m_registry.create(name, attrs);
                    if (action) {
                        nodeStack.back()->m_actions.push_back(action);
                    } else {
                        m_pMsgs->Message("Failed to create action: " + name, false);
                    }
                }
            }
            // Unknown elements are silently ignored
        };

    pugi::xml_node controlElem = document.child("control");
    if (!controlElem) controlElem = document.root().first_child();

    if (controlElem) {
        for (pugi::xml_node child : controlElem.children())
            processElement(child, m_parsedNodes);
    }

    // Resolve forward references
    for (auto& ref : unresolvedRefs) {
        auto target = namedNodes.find(ref.second);
        if (target != namedNodes.end()) *(ref.first) = target->second;
    }

    updateActions();
    return true;
}
