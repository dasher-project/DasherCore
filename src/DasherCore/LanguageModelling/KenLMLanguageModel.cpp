#ifdef DASHER_USE_KENLM

#include "KenLMLanguageModel.h"
#include "../Alphabet/AlphInfo.h"

#include "lm/model.hh"
#include "lm/virtual_interface.hh"
#include "lm/state.hh"
#include "lm/word_index.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

namespace Dasher {

struct CKenLMLanguageModel::KenState {
    std::vector<uint8_t> buf;

    explicit KenState(size_t sz) : buf(sz, 0) {}
    KenState(const KenState &o) : buf(o.buf) {}

    void *data() { return buf.data(); }
    const void *data() const { return buf.data(); }
};

CKenLMLanguageModel::KenState *CKenLMLanguageModel::ctx(Context c) const {
    assert(c != nullContext && c <= m_contexts.size());
    return m_contexts[c - 1];
}

CLanguageModel::Context CKenLMLanguageModel::pushCtx(KenState *s) {
    m_contexts.push_back(s);
    return m_contexts.size();
}

CKenLMLanguageModel::CKenLMLanguageModel(const CAlphInfo *pAlphabet,
                                           const std::string &modelPath,
                                           int iNumSyms)
    : CLanguageModel(iNumSyms),
      m_pAlphabet(pAlphabet)
{
    try {
        lm::ngram::Config cfg;
        cfg.load_method = util::LAZY;
        m_model = lm::ngram::LoadVirtual(modelPath.c_str(), cfg);
        m_stateSize = m_model->StateSize();
        m_loaded = true;
    } catch (const std::exception &) {
        m_model = nullptr;
        m_loaded = false;
        return;
    }

    const lm::base::Vocabulary &vocab = m_model->BaseVocabulary();
    m_symToWord.resize(iNumSyms + 1, vocab.NotFound());
    for (int sym = 1; sym <= iNumSyms; ++sym) {
        const std::string &txt = pAlphabet->GetText(sym);
        if (!txt.empty()) {
            m_symToWord[sym] = vocab.Index(txt);
        }
    }
}

CKenLMLanguageModel::~CKenLMLanguageModel() {
    for (auto *s : m_contexts) delete s;
    delete m_model;
}

CLanguageModel::Context CKenLMLanguageModel::CreateEmptyContext() {
    auto *s = new KenState(m_stateSize);
    if (m_model) {
        m_model->BeginSentenceWrite(s->data());
    }
    return pushCtx(s);
}

CLanguageModel::Context CKenLMLanguageModel::CloneContext(Context c) {
    auto *s = new KenState(*ctx(c));
    return pushCtx(s);
}

void CKenLMLanguageModel::ReleaseContext(Context c) {
    if (c == nullContext) return;
    auto *&p = m_contexts[c - 1];
    delete p;
    p = nullptr;
}

void CKenLMLanguageModel::EnterSymbol(Context c, int Symbol) {
    if (!m_model || Symbol < 1 || Symbol >= (int)m_symToWord.size()) return;
    KenState *in = ctx(c);
    KenState out(m_stateSize);
    lm::WordIndex w = m_symToWord[Symbol];
    m_model->BaseScore(in->data(), w, out.data());
    std::memcpy(in->data(), out.data(), m_stateSize);
}

void CKenLMLanguageModel::LearnSymbol(Context c, int Symbol) {
    EnterSymbol(c, Symbol);
}

void CKenLMLanguageModel::GetProbs(Context c, std::vector<unsigned int> &Probs,
                                    int iNorm, int iUniform) const {
    const int numSyms = GetSize();
    Probs.resize(numSyms, 0);

    if (!m_model) {
        unsigned int each = iNorm / std::max(numSyms - 1, 1);
        for (int i = 1; i < numSyms; ++i) Probs[i] = each;
        return;
    }

    const KenState *inState = ctx(c);

    std::vector<float> logProbs(numSyms, -100.0f);

    KenState tmp(m_stateSize);
    for (int sym = 1; sym < numSyms; ++sym) {
        lm::WordIndex w = (sym < (int)m_symToWord.size()) ? m_symToWord[sym] : m_model->BaseVocabulary().NotFound();
        float score = m_model->BaseScore(inState->data(), w, tmp.data());
        logProbs[sym] = score;
    }

    float maxLogP = *std::max_element(logProbs.begin() + 1, logProbs.end());

    std::vector<double> weights(numSyms, 0.0);
    double total = 0.0;
    for (int i = 1; i < numSyms; ++i) {
        weights[i] = std::pow(10.0, (double)(logProbs[i] - maxLogP));
        total += weights[i];
    }

    const double uniformShare = (double)iUniform / (double)iNorm;
    const double modelShare = 1.0 - uniformShare;
    const double perSymUniform = uniformShare / std::max(numSyms - 1, 1);

    unsigned int assigned = 0;
    for (int i = 1; i < numSyms; ++i) {
        double p = modelShare * (weights[i] / total) + perSymUniform;
        unsigned int q = static_cast<unsigned int>(p * iNorm + 0.5);
        if (q < 1) q = 1;
        Probs[i] = q;
        assigned += q;
    }

    if (assigned != (unsigned int)iNorm) {
        int best = 1;
        for (int i = 2; i < numSyms; ++i) {
            if (Probs[i] > Probs[best]) best = i;
        }
        int diff = (int)iNorm - (int)assigned;
        Probs[best] = std::max(1, (int)Probs[best] + diff);
    }
}

int CKenLMLanguageModel::GetContextLength() const {
    if (m_model) return m_model->Order();
    return 5;
}

}

#endif
