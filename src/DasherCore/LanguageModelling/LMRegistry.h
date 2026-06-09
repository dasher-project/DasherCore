// LMRegistry.h
//
// Language Model Registry for DasherCore.
//
// Provides a plugin-style factory for language models. Built-in LMs
// (PPM, Word, Mixture, CTW) self-register at static init time.
// External LMs (e.g. KenLM, ONNX) can register via Dasher::LMRegistry::registerLM().
//
// See docs/LM_REGISTRY.md for full documentation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "LanguageModel.h"
#include <functional>
#include <string>
#include <vector>
#include <map>

namespace Dasher {

  class CSettingsStore;
  class CAlphInfo;
  class CAlphabetMap;

  struct LMDescriptor {
    int id;
    std::string name;
    std::string description;
    bool needsAlphabetMap;
    bool needsAlphInfo;

    using FactoryFn = std::function<CLanguageModel*(
      CSettingsStore*, const CAlphInfo*, const CAlphabetMap*, int)>;

    FactoryFn factory;
  };

  class LMRegistry {
  public:
    static LMRegistry& instance();

    void registerLM(const LMDescriptor& descriptor);

    CLanguageModel* create(int id,
                           CSettingsStore* settings,
                           const CAlphInfo* alphInfo,
                           const CAlphabetMap* alphMap,
                           int numSyms) const;

    const LMDescriptor* get(int id) const;
    const std::vector<LMDescriptor>& all() const;
    std::vector<int> ids() const;
    int count() const;
    bool has(int id) const;

  private:
    LMRegistry() = default;
    std::map<int, LMDescriptor> m_registry;
    std::vector<LMDescriptor> m_ordered;
  };

}
