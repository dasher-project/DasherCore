// LMRegistry.cpp
//
// Language Model Registry implementation.
//
///////////////////////////////////////////////////////////////////////////////

#include "LMRegistry.h"
#include "PPMLanguageModel.h"
#include "WordLanguageModel.h"
#include "MixtureLanguageModel.h"
#include "CTWLanguageModel.h"
#include "DictLanguageModel.h"
#ifdef DASHER_USE_KENLM
#include "KenLMLanguageModel.h"
#include "../Alphabet/AlphInfo.h"
#include "../FileUtils.h"
#include <fstream>
#endif
#include "../SettingsStore.h"
#include "../Alphabet/AlphInfo.h"
#include "../Parameters.h"

using namespace Dasher;

LMRegistry& LMRegistry::instance() {
  static LMRegistry reg;
  return reg;
}

void LMRegistry::registerLM(const LMDescriptor& descriptor) {
  m_registry[descriptor.id] = descriptor;
  m_ordered.push_back(descriptor);
}

CLanguageModel* LMRegistry::create(int id,
                                    CSettingsStore* settings,
                                    const CAlphInfo* alphInfo,
                                    const CAlphabetMap* alphMap,
                                    int numSyms) const {
  auto it = m_registry.find(id);
  if (it == m_registry.end()) {
    it = m_registry.find(0);
    if (it == m_registry.end()) return nullptr;
  }
  return it->second.factory(settings, alphInfo, alphMap, numSyms);
}

const LMDescriptor* LMRegistry::get(int id) const {
  auto it = m_registry.find(id);
  return it != m_registry.end() ? &it->second : nullptr;
}

const std::vector<LMDescriptor>& LMRegistry::all() const {
  return m_ordered;
}

std::vector<int> LMRegistry::ids() const {
  std::vector<int> result;
  result.reserve(m_ordered.size());
  for (const auto& d : m_ordered) result.push_back(d.id);
  return result;
}

int LMRegistry::count() const {
  return static_cast<int>(m_ordered.size());
}

bool LMRegistry::has(int id) const {
  return m_registry.find(id) != m_registry.end();
}

namespace {

struct StaticInit {
  StaticInit() {
    auto& reg = LMRegistry::instance();

    reg.registerLM({0, "PPM", "Prediction by Partial Match", false, false,
      {LP_LM_ALPHA, LP_LM_BETA, LP_LM_MAX_ORDER, LP_LM_EXCLUSION, LP_LM_UPDATE_EXCLUSION},
      [](CSettingsStore* s, const CAlphInfo*, const CAlphabetMap*, int n) -> CLanguageModel* {
        return new CPPMLanguageModel(s, n);
      }});

    reg.registerLM({2, "Word", "Word-level language model", true, true,
      {LP_LM_WORD_ALPHA, LP_LM_MAX_ORDER},
      [](CSettingsStore* s, const CAlphInfo* a, const CAlphabetMap* m, int) -> CLanguageModel* {
        return new CWordLanguageModel(s, a, m);
      }});

    reg.registerLM({3, "Mixture", "PPM + Dictionary mixture", true, true,
      {LP_LM_ALPHA, LP_LM_BETA, LP_LM_MAX_ORDER, LP_LM_MIXTURE, LP_LM_WORD_ALPHA, LP_LM_EXCLUSION, LP_LM_UPDATE_EXCLUSION},
      [](CSettingsStore* s, const CAlphInfo* a, const CAlphabetMap* m, int) -> CLanguageModel* {
        return new CMixtureLanguageModel(s, a, m);
      }});

    reg.registerLM({4, "CTW", "Context Tree Weighting", false, false,
      {LP_LM_MAX_ORDER},
      [](CSettingsStore*, const CAlphInfo*, const CAlphabetMap*, int n) -> CLanguageModel* {
        return new CCTWLanguageModel(n);
      }});

#ifdef DASHER_USE_KENLM
    reg.registerLM({5, "KenLM", "KenLM character n-gram model (pre-trained)", true, true,
      {LP_UNIFORM},
      [](CSettingsStore*, const CAlphInfo* a, const CAlphabetMap*, int n) -> CLanguageModel* {
        std::string modelPath;
        const std::string baseName = "kenlm_" + a->GetID();
        const std::string genericName = "kenlm";
        const char *exts[] = {".binary", ".bin"};
        bool found = false;
        for (const auto &base : {baseName, genericName}) {
          for (const auto &ext : exts) {
            std::string candidate = Dasher::FileUtils::GetFullFilenamePath(base + ext);
            std::ifstream probe(candidate, std::ios::binary);
            if (probe.is_open()) {
              modelPath = candidate;
              found = true;
              break;
            }
          }
          if (found) break;
        }
        if (!found) return nullptr;
        auto *klm = new CKenLMLanguageModel(a, modelPath, n);
        if (klm->IsLoaded()) return klm;
        delete klm;
        return nullptr;
      }});
#endif
  }
};

static StaticInit s_lmRegistryInit;

}
