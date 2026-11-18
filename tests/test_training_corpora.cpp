// test_training_corpora.cpp — guards for the #119 corpus resolution.
//
// Scripts/resolve-training-corpora.py materializes alias corpus files so
// alphabets whose declared trainingFilename never shipped still train from
// the canonical training_wa_<lang>_<script>.txt corpus for their exact
// (lang, script) pair. These tests pin the properties that resolution must
// never break:
//
//   1. consistency — alphabet_index.json's training_available matches the
//      training directory for every alphabet (no drift in either direction);
//   2. completeness — every still-missing alphabet with lang+script and
//      conversion "none" has NO canonical wa_ corpus for its pair: if one
//      exists, the resolver was not run after new data landed;
//   3. failure is preserved — the known unresolvable families (conversion
//      alphabets, no-lang metadata entries, deliberately unpaired corpora)
//      still report training_available false;
//   4. runtime — a resolved alphabet (Spanish) starts trained and emits no
//      "No training text" message, its NFD combining-accents variant too,
//      while the same alphabet against a data dir WITHOUT the alias still
//      reports the missing corpus exactly as before.
//
// Self-contained: focused line scan of the generated index (same approach
// as test_locales_sync.cpp — no JSON dependency).

#include "test_common.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct IndexEntry {
    std::string id;
    std::string training;
    bool training_available = false;
    bool has_lang = false;
    std::string lang;
    std::string script;
    std::string conversion;
};

// Scan the generated index. Entry objects open with "  {" and their string /
// literal fields sit at exactly 3-space indent; nested array elements are
// deeper and are ignored. The file is generator-owned (no escaped quotes —
// verified in-repo), so plain prefix scans are safe.
std::vector<IndexEntry> scan_index() {
    std::vector<IndexEntry> entries;
    std::ifstream in(fs::path(get_test_data_dir()) / "Data" / "alphabets" / "alphabet_index.json");
    REQUIRE(in.good());
    std::string line;
    std::optional<IndexEntry> cur;
    auto flush = [&] {
        if (cur) entries.push_back(*cur);
        cur.reset();
    };
    while (std::getline(in, line)) {
        if (line.rfind("  {", 0) == 0) {
            flush();
            cur = IndexEntry{};
            continue;
        }
        if (line.rfind("  }", 0) == 0) {
            flush();
            continue;
        }
        if (!cur || line.rfind("   \"", 0) != 0) continue; // 3-space field only
        auto value_of = [&](const char* key) -> std::string {
            const std::string prefix = std::string("   \"") + key + "\": ";
            if (line.rfind(prefix, 0) != 0) return {};
            std::string v = line.substr(prefix.size());
            v.erase(v.find_last_not_of(" \r,") + 1); // trailing comma/space
            return v;
        };
        auto str_field = [&](const char* key) -> std::string {
            std::string v = value_of(key);
            if (v.size() >= 2 && v.front() == '"') return v.substr(1, v.size() - 2);
            return {};
        };
        std::string v;
        if (!(v = value_of("id")).empty())
            cur->id = v.substr(1, v.size() - 2);
        else if (!(v = str_field("training")).empty())
            cur->training = v;
        else if (!(v = value_of("training_available")).empty())
            cur->training_available = (v == "true");
        else if (!(v = value_of("lang")).empty()) {
            if (v != "null") {
                cur->has_lang = true;
                cur->lang = v.substr(1, v.size() - 2);
            }
        } else if (!(v = str_field("script")).empty())
            cur->script = v;
        else if (!(v = str_field("conversion")).empty())
            cur->conversion = v;
    }
    flush();
    return entries;
}

fs::path training_file(const std::string& name) {
    return fs::path(get_test_data_dir()) / "Data" / "training" / name;
}

std::string read_file(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    REQUIRE(in.good());
    return std::string(std::istreambuf_iterator<char>(in), {});
}

std::string canonical_name(const IndexEntry& e) {
    return "training_wa_" + e.lang + "_" + e.script + ".txt";
}

// Message-callback capture for the runtime tests.
struct MessageCapture {
    int missing_training = 0;
    std::string last;
    void clear() {
        missing_training = 0;
        last.clear();
    }
};
void install_message_capture(dasher_ctx* ctx, MessageCapture* cap) {
    dasher_set_message_callback(
        ctx,
        [](int, const char* text, void* ud) {
            auto* c = static_cast<MessageCapture*>(ud);
            if (text && std::string(text).find("No training text") != std::string::npos) {
                c->missing_training++;
                c->last = text;
            }
        },
        cap);
}

} // namespace

TEST(training_corpora_index_consistency) {
    auto entries = scan_index();
    ASSERT(entries.size() > 300); // full shipped index, not a stub

    int checked = 0, available = 0, consistent = 0;
    for (const auto& e : entries) {
        if (e.training.empty()) continue; // no declared corpus: not our concern
        checked++;
        const bool exists = fs::exists(training_file(e.training));
        if (e.training_available == exists) consistent++;
        if (e.training_available) available++;
        if (e.training_available != exists)
            printf("  DRIFT: %s wants %s, available flag=%d, file exists=%d\n", e.id.c_str(), e.training.c_str(),
                   e.training_available, exists);
    }
    printf("  alphabets with declared training: %d, %d consistent, %d marked available\n", checked, consistent,
           available);
    ASSERT(checked > 150);
    ASSERT_EQ(consistent, checked);

    // The summaries block counts the same flags and must not disagree with
    // the entries (it drifted once: 151/89 stale after 43 flips).
    const fs::path index_path = fs::path(get_test_data_dir()) / "Data" / "alphabets" / "alphabet_index.json";
    std::string raw = read_file(index_path);
    auto summary_count = [&](const char* key) -> int {
        const std::string needle = std::string("  \"") + key + "\": ";
        const auto pos = raw.find(needle);
        if (pos == std::string::npos) return -1;
        return atoi(raw.c_str() + pos + needle.size());
    };
    const int sum_available = summary_count("training_available");
    const int sum_missing = summary_count("training_missing");
    printf("  summary counts: available=%d (entries %d), missing=%d (entries %d)\n", sum_available, available,
           sum_missing, checked - available);
    if (sum_available >= 0) ASSERT_EQ(sum_available, available);
    if (sum_missing >= 0) ASSERT_EQ(sum_missing, checked - available);
}

TEST(training_corpora_alias_files_normalized_as_declared) {
    // Data-level guard: an NFC alias must be byte-identical to its canonical
    // source, and a combining-accents (NFD) alias must actually be
    // decomposed — a regression that swaps in the NFC copy would leave the
    // alphabet silently near-untrained with no runtime message to catch it.
    struct Pair {
        const char* alias;
        const char* source;
        bool nfd;
    };
    static const Pair pairs[] = {
        {"training_spanish_ES.txt", "training_wa_es_Latn.txt", false},
        {"training_spanishC_ES.txt", "training_wa_es_Latn.txt", true},
        {"training_catalanC_ES.txt", "training_wa_ca_Latn.txt", true},
        {"training_italianC_IT.txt", "training_wa_it_Latn.txt", true},
        {"training_italian_IT.txt", "training_wa_it_Latn.txt", false},
        {"training_portugueseC_BR.txt", "training_wa_pt_Latn.txt", true},
        {"training_portuguese_BR.txt", "training_wa_pt_Latn.txt", false},
        {"training_hiragana60_JP.txt", "training_wa_ja_Hira.txt", false},
        {"training_german_DE.txt", "training_wa_de_Latn.txt", false},
    };
    for (const auto& p : pairs) {
        const std::string alias = read_file(training_file(p.alias));
        const std::string source = read_file(training_file(p.source));
        if (p.nfd) {
            if (alias == source) printf("  %s is a byte-copy of %s — expected NFD normalization\n", p.alias, p.source);
            ASSERT(alias != source);
            // Decomposed text carries UTF-8 combining marks (U+0300..U+036F
            // encode as 0xCC/0xCD lead bytes).
            const bool has_combining =
                alias.find("\xCC") != std::string::npos || alias.find("\xCD") != std::string::npos;
            if (!has_combining) printf("  %s carries no combining marks\n", p.alias);
            ASSERT(has_combining);
        } else {
            if (alias != source) printf("  %s differs from %s — expected byte copy\n", p.alias, p.source);
            ASSERT(alias == source);
        }
    }
}

TEST(training_corpora_resolution_complete) {
    // Every alphabet that could be resolved MUST be resolved: if the declared
    // file is missing but a canonical wa_ corpus exists for the alphabet's
    // exact (lang, script) pair, the resolver was not run after the data
    // landed — fail so the aliases get refreshed.
    auto entries = scan_index();
    int unresolved = 0, resolvable_left = 0;
    for (const auto& e : entries) {
        if (e.training.empty() || fs::exists(training_file(e.training))) continue;
        unresolved++;
        if (!e.has_lang || e.script.empty() || e.conversion != "none") continue;
        if (fs::exists(training_file(canonical_name(e)))) {
            printf("  RESOLVABLE BUT MISSING: %s wants %s, canonical %s ships\n", e.id.c_str(), e.training.c_str(),
                   canonical_name(e).c_str());
            resolvable_left++;
        }
    }
    printf("  still-missing alphabets: %d, of which resolvable: %d\n", unresolved, resolvable_left);
    ASSERT_EQ(resolvable_left, 0);
    ASSERT(unresolved > 20); // genuinely unpaired corpora must remain
}

TEST(training_corpora_missing_still_reported) {
    // Sentinels for the unresolvable families: conversion alphabets,
    // no-lang metadata, and corpora that simply do not exist anywhere.
    // If any of these ever flips to available, the resolution overreached.
    // (Substring match: generated ids carry suffixes like " (5 groups)".)
    static const char* sentinels[] = {
        "Chinese Pin Yin with tones (no chinese characters)",
        "_ Klingon / tlhIngan Hol",
        "International Phonetic Alphabet",
        "Pure Katakana, halfwidths, many smalls", // lang metadata is wrong (ak) — must fail closed
        "Mandarin (simp) by Pinyin, tone after first vowel",
        "Abc (Musical language)",
    };
    auto entries = scan_index();
    for (const char* id : sentinels) {
        bool matched = false;
        for (const auto& e : entries) {
            if (e.id.find(id) == std::string::npos) continue;
            matched = true;
            if (e.training_available) {
                printf("  OVERREACH: %s resolved but should not be\n", e.id.c_str());
                ASSERT(!e.training_available);
            }
        }
        if (!matched) printf("  sentinel missing from index: %s\n", id);
        ASSERT(matched);
    }
}

TEST(training_spanish_resolves_at_runtime) {
    // NOTE: the ñ below is a raw UTF-8 source literal, deliberately not a
    // \u escape — MSVC (no /utf-8 in this project) encodes \u00f1 as a single
    // CP1252 byte, breaking the API's UTF-8 contract on Windows. Raw UTF-8
    // round-trips unchanged; same pattern as test_multilingual.cpp.
    MessageCapture cap;
    ScopedContext ctx(800, 600);
    install_message_capture(ctx, &cap);
    cap.clear();

    dasher_set_alphabet_id(ctx, "Español / Spanish with punctuation and numerals");
    run_frames(ctx, 3);

    printf("  missing-training messages: %d\n", cap.missing_training);
    ASSERT_EQ(cap.missing_training, 0);
    // And the model actually trained: root children carry probability mass
    // (an untrained model reports the alphabet's symbols with near-uniform
    // mass, but a broken selection would leave the message above).
    int lbnds[64] = {0}, hbnds[64] = {0};
    const int count = dasher_get_probabilities(ctx, lbnds, hbnds, 64);
    printf("  root children with probabilities: %d\n", count);
    ASSERT(count > 0);
}

TEST(training_spanish_combining_nfd_resolves_at_runtime) {
    MessageCapture cap;
    ScopedContext ctx(800, 600);
    install_message_capture(ctx, &cap);
    cap.clear();

    // The NFD variant trains from the NFD-normalized alias
    // (training_spanishC_ES.txt); a normalization mismatch would leave it
    // effectively untrained — the missing-training message stays the signal.
    dasher_set_alphabet_id(ctx, "Español / Spanish  (combining accents) with numerals and punctuation");
    run_frames(ctx, 3);
    ASSERT_EQ(cap.missing_training, 0);
}

TEST(training_missing_corpus_still_reports_at_runtime) {
    // The same Spanish alphabet against a data dir WITHOUT the alias file
    // must report the missing corpus exactly as before resolution existed:
    // failure reporting for genuinely missing corpora is preserved.
    // (Recursive symlink copy: build_data_dir() only links the top level of
    // each subdir, and the Spanish definition lives in autoConverted/.)
    ScopedTempDir tmp;
    fs::path root = fs::path(tmp.path);
    std::error_code ec;
    fs::copy(fs::path(get_test_data_dir()) / "Data", root / "Data",
             fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    ASSERT(fs::exists(root / "Data" / "alphabets" / "autoConverted"));
    fs::remove(root / "Data" / "training" / "training_spanish_ES.txt", ec);
    ASSERT(!fs::exists(root / "Data" / "training" / "training_spanish_ES.txt"));

    MessageCapture cap;
    dasher_ctx* ctx = dasher_create(root.string().c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    install_message_capture(ctx, &cap);
    cap.clear();

    dasher_set_alphabet_id(ctx, "Español / Spanish with punctuation and numerals");
    run_frames(ctx, 3);
    printf("  missing-training messages: %d last='%s'\n", cap.missing_training, cap.last.c_str());
    ASSERT_EQ(cap.missing_training, 1);
    dasher_destroy(ctx);
}
