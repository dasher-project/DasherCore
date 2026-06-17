// ControlManager.h
//
// Unified action system + control node tree for Dasher.
//
// This file replaces the old Actions.h / ActionManager.h system with a single,
// generic, extensible action framework. Both alphabet symbol nodes and control
// nodes create and execute actions through the same ActionRegistry.
//
// Copyright (c) 2007-2024 The Dasher Team
//
// This file is part of Dasher. Dasher is free software; you can redistribute
// it and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.

#pragma once

#include "AbstractXMLParser.h"
#include "DasherNode.h"
#include "NodeManager.h"
#include "Parameters.h"
#include "SettingsStore.h"

#include <functional>
#include <list>
#include <map>
#include <string>
#include <variant>
#include <vector>

class CNodeCreationManager;

namespace Dasher {

class CDasherInterfaceBase;
class CControlManager;

// ── EditDistance ───────────────────────────────────────────────────────────
// Moved here from Actions.h. Used by move/delete/speak/copy actions.

typedef enum EditDistance : unsigned int {
    EDIT_CHAR,
    EDIT_WORD,
    EDIT_SENTENCE,
    EDIT_PARAGRAPH,
    EDIT_FILE,
    EDIT_LINE,
    EDIT_PAGE,
    EDIT_SELECTION,
    EDIT_ALL,
    EDIT_NONE
} EditDistance;

// ── Action system ──────────────────────────────────────────────────────────

/// Base class for all actions. An action is executed when a node is entered
/// (Do) or potentially reversed (Undo). Actions are created by the
/// ActionRegistry from XML attributes.
class ControlAction {
  public:
    virtual ~ControlAction() = default;

    /// Called when the node containing this action is entered.
    virtual void execute(CDasherInterfaceBase *intf) = 0;

    /// Compute the new text offset after this action runs.
    /// Default: offset unchanged.
    virtual int calculateNewOffset(CDasherInterfaceBase *intf, int offsetBefore);
};

/// Factory function type: creates a ControlAction from XML key-value attributes.
using ActionFactory = std::function<ControlAction *(const std::map<std::string, std::string> &)>;

/// Type for custom action callbacks registered by frontends.
/// Receives the action name and all XML attributes.
using CustomActionCallback =
    std::function<void(const std::string &name, const std::map<std::string, std::string> &attrs)>;

/// Wraps a frontend-registered custom action into a ControlAction.
class CustomAction : public ControlAction {
  public:
    CustomAction(const std::string &name, const std::map<std::string, std::string> &attrs,
                 CustomActionCallback callback)
        : m_name(name), m_attrs(attrs), m_callback(std::move(callback)) {}

    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_name;
    std::map<std::string, std::string> m_attrs;
    CustomActionCallback m_callback;
};

/// Registry of action factories. Maps action names (as they appear in XML)
/// to factory functions. Both control.xml and alphabet.xml use this.
class ActionRegistry {
  public:
    ActionRegistry() = default;

    /// Register a factory for a named action type.
    void registerFactory(const std::string &name, ActionFactory factory);

    /// Register a custom action from the C API.
    void registerCustomAction(const std::string &name, CustomActionCallback callback);

    /// Create an action instance from XML name + attributes.
    /// Returns nullptr if the name is not registered.
    ControlAction *create(const std::string &name, const std::map<std::string, std::string> &attrs) const;

    /// Check if a name is registered.
    bool hasAction(const std::string &name) const;

  private:
    struct FactoryEntry {
        ActionFactory factory;
        CustomActionCallback customCallback; // if non-null, wraps as CustomAction
    };
    std::map<std::string, FactoryEntry> m_factories;
};

// ── Built-in action classes ────────────────────────────────────────────────
// These call CDasherInterfaceBase methods directly, no event system needed.

/// Stop Dasher and trigger on-stop behaviour.
class StopAction : public ControlAction {
  public:
    void execute(CDasherInterfaceBase *intf) override;
};

/// Pause Dasher motion.
class PauseAction : public ControlAction {
  public:
    void execute(CDasherInterfaceBase *intf) override;
};

/// Move the cursor forward or backward by a given distance.
class MoveAction : public ControlAction {
  public:
    MoveAction(bool bForwards, EditDistance dist) : m_bForwards(bForwards), m_dist(dist) {}

    int calculateNewOffset(CDasherInterfaceBase *intf, int offsetBefore) override;
    void execute(CDasherInterfaceBase *intf) override;

  private:
    bool m_bForwards;
    EditDistance m_dist;
};

/// Delete text forward or backward by a given distance.
class DeleteAction : public ControlAction {
  public:
    DeleteAction(bool bForwards, EditDistance dist) : m_bForwards(bForwards), m_dist(dist) {}

    int calculateNewOffset(CDasherInterfaceBase *intf, int offsetBefore) override;
    void execute(CDasherInterfaceBase *intf) override;

  private:
    bool m_bForwards;
    EditDistance m_dist;
};

/// Base for actions that use text context (speak/copy).
class TextActionBase : public ControlAction {
  public:
    enum ActionContext { Repeat, NewText, Distance };

    TextActionBase(ActionContext context, EditDistance dist) : m_context(context), m_dist(dist) {}

  protected:
    std::string getText(CDasherInterfaceBase *intf);

    ActionContext m_context;
    EditDistance m_dist;
    int m_iStartOffset = 0;
    std::string m_strLast;
};

/// Speak text based on context mode.
class SpeakAction : public TextActionBase {
  public:
    using TextActionBase::TextActionBase;
    void execute(CDasherInterfaceBase *intf) override;
};

/// Cancel ongoing speech.
class SpeakCancelAction : public ControlAction {
  public:
    void execute(CDasherInterfaceBase *intf) override;
};

/// Copy text to clipboard based on context mode.
class CopyAction : public TextActionBase {
  public:
    using TextActionBase::TextActionBase;
    void execute(CDasherInterfaceBase *intf) override;
};

/// Output a fixed text character (the default behaviour for alphabet symbols).
class TextOutputAction : public ControlAction {
  public:
    TextOutputAction(std::string text) : m_text(std::move(text)) {}
    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_text;
};

/// Delete a fixed text character (undo for alphabet symbols).
class TextDeleteAction : public ControlAction {
  public:
    TextDeleteAction(std::string text) : m_text(std::move(text)) {}
    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_text;
};

/// Speak a fixed string.
class FixedSpeakAction : public ControlAction {
  public:
    FixedSpeakAction(std::string text) : m_text(std::move(text)) {}
    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_text;
};

/// Change a Dasher setting (deferred to end of frame).
class ChangeSettingAction : public ControlAction {
  public:
    ChangeSettingAction(Parameter setting, std::variant<bool, long, std::string> newValue)
        : m_parameter(setting), m_newValue(std::move(newValue)) {}

    void execute(CDasherInterfaceBase *intf) override;

  private:
    Parameter m_parameter;
    std::variant<bool, long, std::string> m_newValue;
};

/// Send keyboard key events (for ATSPI / accessibility integration).
class KeyboardAction : public ControlAction {
  public:
    enum PressType { KEY_PRESS, KEY_RELEASE, KEY_PRESS_RELEASE };

    KeyboardAction(PressType type, std::vector<std::vector<unsigned short>> keycodes)
        : m_type(type), m_keycodes(std::move(keycodes)) {}

    void execute(CDasherInterfaceBase *intf) override;

  private:
    PressType m_type;
    std::vector<std::vector<unsigned short>> m_keycodes;
};

/// Output to a socket.
class SocketOutputAction : public ControlAction {
  public:
    SocketOutputAction(std::string socketName, std::string action, bool addNewLine)
        : m_socketName(std::move(socketName)), m_action(std::move(action)), m_addNewLine(addNewLine) {}

    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_socketName;
    std::string m_action;
    bool m_addNewLine;
};

/// ATSPI accessibility action.
class ATSPIAction : public ControlAction {
  public:
    ATSPIAction(std::string action) : m_action(std::move(action)) {}
    void execute(CDasherInterfaceBase *intf) override;

  private:
    std::string m_action;
};

// ── Control node tree ──────────────────────────────────────────────────────

/// Template describing a control node: label, colour, successors, and actions.
/// NodeTemplates form a shared directed graph via the successors list.
/// A nullptr entry in successors means an <alph/> escape (bridge to alphabet).
class NodeTemplate {
  public:
    NodeTemplate(const std::string &strLabel, int iColour)
        : m_strLabel(strLabel), m_iColour(iColour), m_pLabel(nullptr) {}

    ~NodeTemplate() {
        delete m_pLabel;
        for (auto *action : m_actions)
            delete action;
    }

    const std::string m_strLabel;
    const int m_iColour;
    std::list<NodeTemplate *> successors; ///< nullptr = <alph/> escape
    std::vector<ControlAction *> m_actions;
    CDasherScreen::Label *m_pLabel;

    int calculateNewOffset(CDasherInterfaceBase *intf, int offsetBefore) const;
    void executeActions(CDasherInterfaceBase *intf) const;
};

/// A node in the control tree. Children distributed uniformly (no LM).
class CContNode : public CDasherNode {
  public:
    CContNode(int iOffset, int iColour, NodeTemplate *pTemplate, CControlManager *pMgr);

    CNodeManager *mgr() const override;
    CDasherScreen::Label *getLabel() override { return m_pTemplate->m_pLabel; }
    bool bShove() override { return false; }
    double SpeedMul() override;

    void PopulateChildren() override;
    int ExpectedNumChildren() override;
    void Do() override;

    const ColorPalette::Color &getLabelColor(const ColorPalette *) override { return ColorPalette::noColor; }
    const ColorPalette::Color &getOutlineColor(const ColorPalette *) override { return ColorPalette::noColor; }
    const ColorPalette::Color &getNodeColor(const ColorPalette *) override;

    NodeTemplate *templateNode() const { return m_pTemplate; }
    int colourIndex() const { return m_iColourIndex; }

  private:
    NodeTemplate *m_pTemplate;
    CControlManager *m_pMgr;
    int m_iColourIndex;
};

/// Node manager for control nodes. Owns the action registry, the root
/// template, and parses control.xml.
class CControlManager : public CNodeManager, public AbstractXMLParser {
  public:
    CControlManager(CSettingsStore *pSettingsStore, CDasherInterfaceBase *pInterface,
                    CNodeCreationManager *pNCManager, CMessageDisplay *pMsgs);
    ~CControlManager();

    /// Create a root control node for grafting under an alphabet node.
    /// Returns nullptr if no root template is configured.
    CDasherNode *GetRoot(CDasherNode *pContext, int iOffset);

    /// Create/update screen labels for all templates.
    void ChangeScreen(CDasherScreen *pScreen);

    /// Pick a colour index for a template, auto-cycling if unspecified.
    int getColour(NodeTemplate *pTemplate, CDasherNode *pParent);

    /// Rebuild root template's successors from parsed XML.
    void updateActions();

    // Accessors
    CDasherInterfaceBase *GetInterface() { return m_pInterface; }
    CSettingsStore *GetSettingsStore() { return m_pSettingsStore; }
    CNodeCreationManager *GetNCManager() { return m_pNCManager; }
    NodeTemplate *GetRootTemplate() { return m_pRoot; }
    ActionRegistry *GetActionRegistry() { return &m_registry; }

    // AbstractXMLParser
    bool Parse(pugi::xml_document &document, const std::string filePath, bool bUser) override;

  private:
    CSettingsStore *m_pSettingsStore;
    CDasherInterfaceBase *m_pInterface;
    CNodeCreationManager *m_pNCManager;
    CDasherScreen *m_pScreen = nullptr;

    ActionRegistry m_registry;
    NodeTemplate *m_pRoot = nullptr;
    std::list<NodeTemplate *> m_parsedNodes;

    /// Register all built-in actions in the registry.
    void registerBuiltinActions();

  public:
    /// Map a colour index to a ColorPalette::Color.
    static const ColorPalette::Color &indexToColor(int iIndex);
};

} // namespace Dasher
