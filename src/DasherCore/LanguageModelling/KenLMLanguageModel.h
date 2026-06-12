#pragma once

#include "LanguageModel.h"

#include <memory>
#include <string>
#include <vector>

namespace lm { namespace base { class Model; } }

namespace Dasher {

class CAlphInfo;

class CKenLMLanguageModel : public CLanguageModel {
public:
    CKenLMLanguageModel(const CAlphInfo *pAlphabet,
                        const std::string &modelPath,
                        int iNumSyms);

    ~CKenLMLanguageModel() override;

    Context CreateEmptyContext() override;
    Context CloneContext(Context ctx) override;
    void ReleaseContext(Context ctx) override;
    void EnterSymbol(Context ctx, int Symbol) override;
    void LearnSymbol(Context ctx, int Symbol) override;
    void GetProbs(Context ctx, std::vector<unsigned int> &Probs,
                  int iNorm, int iUniform) const override;
    int GetContextLength() const override;

    bool IsLoaded() const { return m_loaded; }

private:
    struct KenState;

    lm::base::Model *m_model = nullptr;
    const CAlphInfo *m_pAlphabet;
    bool m_loaded = false;
    size_t m_stateSize = 0;

    std::vector<unsigned int> m_symToWord;
    std::vector<KenState *> m_contexts;

    KenState *ctx(Context c) const;
    Context pushCtx(KenState *s);
};

}
