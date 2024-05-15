#pragma once

#include "DasherNode.h"
#include "NodeManager.h"
#include "NodeCreationManager.h"

#include <list>
#include <vector>
#include <map>
#include <string>

#include "Actions.h"

class CNodeCreationManager;

namespace Dasher {
    class CContNode;
    class CDasherInterfaceBase;
    class CControlBase;

  /// \ingroup Model
  /// @{

  class NodeTemplate : public Action {
    public:
      NodeTemplate(const std::string &strLabel, int iColour);
      virtual ~NodeTemplate();
      const std::string m_strLabel;
      const int m_iColour;
      std::list<NodeTemplate *> successors;

    private:
      friend class CControlBase;
      friend class CContNode;
      CDasherScreen::Label *m_pLabel;
   };

    /// A node manager which deals with control nodes.
  ///
  class CControlBase : public CNodeManager {
  public:

    NodeTemplate *GetRootTemplate();

    CControlBase(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface, CNodeCreationManager *pNCManager);

    ///Make this manager ready to make nodes renderable on the screen by preallocating labels
    virtual void ChangeScreen(CDasherScreen *pScreen);
    
    ///
    /// Get a new root node owned by this manager
    ///

    virtual CDasherNode *GetRoot(CDasherNode *pContext, int iOffset);
    CDasherInterfaceBase* GetDasherInterface() { return m_pInterface; }
  protected:
      friend class CContNode;
    ///Sets the root - should be called by subclass constructor to make
    /// superclass ready for use.
    ///Note, may only be called once, and with a non-null pRoot, or will throw an error message.
    void SetRootTemplate(NodeTemplate *pRoot);

    CDasherInterfaceBase *m_pInterface;
    CNodeCreationManager *m_pNCManager;
    
    int getColour(NodeTemplate *pTemplate, CDasherNode *pParent);
    CDasherScreen *m_pScreen;
    CSettingsStore* m_pSettingsStore;
  private:
    NodeTemplate *m_pRoot;
  };

  class CContNode : public CDasherNode {
    public:
      CControlBase* mgr() const override {return m_pMgr;}
      CContNode(int iOffset, NodeTemplate *pTemplate, CControlBase *pMgr);
      CDasherScreen::Label *getLabel() override { return m_pTemplate->m_pLabel; }

      bool bShove() override {return false;}
      double SpeedMul() override;
      ///
      /// Provide children for the supplied node
      ///

      virtual void PopulateChildren() override;
      virtual int ExpectedNumChildren() override;

      virtual void Output() override;
      const ColorPalette::Color& getLabelColor(const ColorPalette* colorPalette) override;
      const ColorPalette::Color& getOutlineColor(const ColorPalette* colorPalette) override;
      const ColorPalette::Color& getNodeColor(const ColorPalette* colorPalette) override;

  private:
      NodeTemplate *m_pTemplate;
      CControlBase *m_pMgr;
    };

///Class reads node tree definitions from an XML file, linking together the NodeTemplates
/// according to defined names, nesting of <node/>s, and  <ref/>s. Also handles the
/// <alph/> tag, meaning one child of the node is to escape back to the alphabet. Subclasses
/// may override parseAction to provide actions for the nodes to perform, also parseOther
/// to link with NodeTemplates from other sources.
class CControlParser : public AbstractXMLParser
{
public:
	CControlParser(CMessageDisplay* pMsgs);
private:
	void ParseNodeRecursive(pugi::xml_node node, std::list<NodeTemplate*>& parent);
public:
	///Loads all node definitions from the specified filename. Note that
	/// system files will not be loaded if user files are (and user files will
	/// clear out any nodes from system ones). However, multiple system or multiple
	/// user files, will be concatenated. (However, files are processed separately:
	/// e.g. names defined in one file will not be seen from another)
	bool Parse(pugi::xml_document& document, const std::string filePath, bool bUser) override;
protected:
	/// \return all node definitions that have been loaded by this CControlParser.
	const std::list<NodeTemplate*>& parsedNodes();
	///Subclasses may override to parse other nodes (besides "node", "ref" and "alph").
	///The default implementation always returns NULL.
	/// \return A node template, if the name was recognised; NULL if not recognised.
	virtual NodeTemplate* parseOther(pugi::xml_node node)
	{
		return nullptr;
	}

	///Subclasses may override to parse actions within nodes.
	///The default implementation always returns NULL.
	/// \return A (new) action pointer, if the name+attributes were successfully parsed; NULL if not recognised.
	virtual Action* parseAction(pugi::xml_node node)
	{
		return nullptr;
	};
private:
	///all top-level parsed nodes
	std::list<NodeTemplate*> m_vParsed;
	///whether parsed nodes were from user file or not
	bool m_bUser;

	///Following only used as temporary variables during parsing...
	std::map<std::string, NodeTemplate*> namedNodes;
	std::list<std::pair<NodeTemplate**, std::string>> unresolvedRefs;
};

  ///subclass which we actually construct! Parses editing node definitions from a file,
  /// then adds Pause and/or Stop, Speak, and Copy (to clipboard), all as children
  /// of the "root" control node.
  class CControlManager : public CControlBase, public CControlParser {
  public:
    CControlManager(CSettingsStore* pSettingsStore, CNodeCreationManager *pNCManager, CDasherInterfaceBase *pInterface);

    ///Recomputes which of pause, stop, speak and copy the root control node should have amongst its children.
    /// Automatically called whenever copy-on-stop/speak-on-stop or input filter changes;
    /// subclasses of CDasherInterfaceBase should also call this if
    ///  (a) they override Stop() and hasStopTriggers() with additional actions, if these are enabled/disabled
    ///      and this causes the value returned by hasStopTriggers() to change;
    ///  (b) the values returned by SupportsSpeech() and/or SupportsClipboard() ever change.
    void updateActions();
    ~CControlManager();

  protected:
    ///Override to allow a <root/> tag to include a fresh control root
    NodeTemplate *parseOther(pugi::xml_node node) override;
    ///Override to recognise <move/> and <delete/> tags as actions.
    Action *parseAction(pugi::xml_node node) override;

  private:
    std::map<std::string, Dasher::Action*> m_actions;
  };
  /// @}

class CControlBoxIO : public AbstractXMLParser
{
public:
	CControlBoxIO(CMessageDisplay* pMsgs);

	void GetControlBoxes(std::vector<std::string>* pList) const;
	CControlManager* CreateControlManager(const std::string& id, CSettingsStore* pSettingsStore, CNodeCreationManager* pNCManager, CDasherInterfaceBase*
                                          pInterface) const;
	bool Parse(pugi::xml_document& document, const std::string filePath, bool bUser) override;
private:
	std::map<std::string, std::string> m_controlFiles;
	std::string m_filename;
};

}
