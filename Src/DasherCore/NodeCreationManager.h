#ifndef __NodeCreationManager_h__
#define __NodeCreationManager_h__

#include "AlphabetManager.h"
#include "ConversionManager.h"
#include "Trainer.h"
#include "SettingsStore.h"

#include <string>

namespace Dasher {
  class CDasherNode;
  class CDasherInterfaceBase;
  class CDasherScreen;
  class CControlBoxIO;
}

//TODO why is CNodeCreationManager _not_ in namespace Dasher?!?!
/// \ingroup Model
/// @{
class CNodeCreationManager {
 public:
  CNodeCreationManager(Dasher::CSettingsStore* pSettingsStore,
                       Dasher::CDasherInterfaceBase *pInterface,
                       const Dasher::CAlphIO *pAlphIO);
  ~CNodeCreationManager();
  
  ///Tells us the screen on which all created node labels must be rendered
  void ChangeScreen(Dasher::CDasherScreen *pScreen);
  
  void HandleParameterChange(Dasher::Parameter parameter) {}
  ///
  /// Get a root node of a particular type
  ///

  Dasher::CAlphabetManager *GetAlphabetManager() {return m_pAlphabetManager;}
    
  ///
  /// Get a reference to the current alphabet
  ///

  const Dasher::CAlphInfo *GetAlphabet() const {
    return m_pAlphabetManager->GetAlphabet();
  }

  void ImportTrainingText(const std::string &strPath);

private:
  Dasher::CTrainer *m_pTrainer;
  
  Dasher::CDasherInterfaceBase *m_pInterface;
  
  Dasher::CAlphabetManager *m_pAlphabetManager;
  
  
  
  ///Screen to use to create node labels
  Dasher::CDasherScreen *m_pScreen;

  Dasher::CSettingsStore* m_pSettingsStore;
};
/// @}

#endif
