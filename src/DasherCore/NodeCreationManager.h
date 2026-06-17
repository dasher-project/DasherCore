#pragma once

#include "AlphabetManager.h"
#include "ConversionManager.h"
#include "DasherModel.h"
#include "Trainer.h"
#include "SettingsStore.h"

#include <string>

namespace Dasher {
class CDasherNode;
class CDasherInterfaceBase;
class CDasherScreen;
class CControlManager;
class CControlBoxIO;
} // namespace Dasher

// TODO why is CNodeCreationManager _not_ in namespace Dasher?!?!
/// \ingroup Model
/// @{
class CNodeCreationManager {
  public:
    CNodeCreationManager(Dasher::CSettingsStore* pSettingsStore, Dasher::CDasherInterfaceBase* pInterface,
                         const Dasher::CAlphIO* pAlphIO);
    ~CNodeCreationManager();

    /// Tells us the screen on which all created node labels must be rendered
    void ChangeScreen(Dasher::CDasherScreen* pScreen);

    void HandleParameterChange(Dasher::Parameter parameter);
    ///
    /// Get a root node of a particular type
    ///

    Dasher::CAlphabetManager* GetAlphabetManager() { return m_pAlphabetManager; }

    Dasher::CControlManager* GetControlManager() { return m_pControlManager; }

    ///
    /// Get a reference to the current alphabet
    ///

    const Dasher::CAlphInfo* GetAlphabet() const { return m_pAlphabetManager->GetAlphabet(); }

    void ImportTrainingText(const std::string& strPath);

    /// Amount of probability space assigned to alphabet nodes
    /// (NORMALIZATION minus control node space, if control mode is on).
    unsigned long GetAlphNodeNormalization() { return m_iAlphNorm; }

    /// Called from AlphabetManager::IterateChildGroups to add the control node
    /// as an extra sibling at the base group level.
    void AddExtras(Dasher::CDasherNode* pParent);

  private:
    Dasher::CTrainer* m_pTrainer;
    Dasher::CDasherInterfaceBase* m_pInterface;
    Dasher::CAlphabetManager* m_pAlphabetManager;
    Dasher::CControlManager* m_pControlManager = nullptr;

    /// Probability space for alphabet nodes (after subtracting control space)
    unsigned long m_iAlphNorm = Dasher::CDasherModel::NORMALIZATION;

    /// Screen to use to create node labels
    Dasher::CDasherScreen* m_pScreen;
    Dasher::CSettingsStore* m_pSettingsStore;

    /// Create or destroy the control manager based on BP_CONTROL_MODE.
    void CreateControlBox();
};
/// @}
