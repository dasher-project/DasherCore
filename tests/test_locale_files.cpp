// test_locale_files.cpp — corpus guard for the strings files (todo.md 5.1).
//
// The engine reads Strings/strings_<locale>.json with a small flat-reader
// (CAPI_locale.cpp — see its header comment for why that is deliberate).
// These tests make that decision safe:
//   1. every SHIPPED locale file loads through the public API without
//      blowing up, and yields the expected key shape;
//   2. escape handling is correct on a fixture file exercising the full
//      JSON escape set (the original reader passed escapes through
//      verbatim — "\n" parsed as 'n', and \" broke framing);
//   3. a malformed file degrades to "no translations", never a crash.
//
// A malformed or format-drifting translation drop therefore fails here,
// at the door, instead of reaching users.

#include "test_common.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace {
// Data dir for fixture-based tests: real bundled data (symlinked) plus a
// Strings/ directory we control. Returns the root to pass to dasher_create.
std::string build_locale_data_dir(const ScopedTempDir& tmp, const std::string& fixture) {
    std::filesystem::path root = tmp.path;
    // Reuse the canonical helper for the engine data (alphabets etc.)…
    std::string engine_root = build_data_dir(tmp);
    (void)engine_root; // …which symlinks into tmp/Data; Strings goes beside it.
    std::filesystem::path strings = root / "Strings";
    std::error_code ec;
    std::filesystem::create_directories(strings, ec);
    std::ofstream out(strings / "strings_zz.json");
    out << fixture;
    return root.string();
}
} // namespace

TEST(locale_corpus_loads) {
    // Every strings_<code>.json in the shipped bundle must load through the
    // public path (dasher_set_locale) and yield the manifest key shape.
    const std::string strings_dir = std::string(get_test_data_dir()) + "/Strings";
    REQUIRE(std::filesystem::is_directory(strings_dir));

    int loaded = 0, non_english = 0;
    for (auto& entry : std::filesystem::directory_iterator(strings_dir)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("strings_", 0) != 0 || name.substr(name.size() - 5) != ".json") continue;
        const std::string code = name.substr(8, name.size() - 8 - 5);
        if (code == "en") continue; // "en" is the reset sentinel, not a file load

        ScopedContext ctx;
        ASSERT_EQ(dasher_set_locale(ctx, code.c_str()), 0);
        loaded++;
        non_english++;
        // A translated parameter label must resolve to SOMETHING non-empty
        // for at least one key (files carry 100+ entries; a parse that
        // silently produced an empty map fails here).
        const char* sample = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
        const char* sample2 = dasher_get_localized_string(ctx, "BP_START_MOUSE.label");
        ASSERT(sample != nullptr || sample2 != nullptr);
        ASSERT(loaded > 0);
    }
    // locales.json (RFC 0003) claims ~35 locales; a wholesale parse failure
    // would show up as far fewer successful loads.
    ASSERT(non_english >= 30);

    // Reset for any later cases in this binary (locale state is global
    // today — see todo.md 5.2).
    ScopedContext reset;
    ASSERT_EQ(dasher_set_locale(reset, "en"), 0);
}

TEST(locale_escape_fixture) {
    // Full escape set incl. \uXXXX (BMP + surrogate pair). Written as a
    // C++ raw string so the JSON escapes survive verbatim into the file.
    const std::string fixture = R"({
    "esc.key": "line1\nline2 \"quoted\" back\\slash tab\there",
    "esc.bullet": "caf\u00e9",
    "esc.astral": "\ud83d\ude00 emoji",
    "esc.unknown": "keep \q as-is",
    "plain.key": "no escapes é raw UTF-8"
}
)";
    ScopedTempDir tmp;
    const std::string root = build_locale_data_dir(tmp, fixture);
    dasher_ctx* ctx = dasher_create(root.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx != nullptr);
    ASSERT_EQ(dasher_set_locale(ctx, "zz"), 0);

    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "esc.key"), "line1\nline2 \"quoted\" back\\slash tab\there");
    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "esc.bullet"), "café");
    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "esc.astral"), "\xF0\x9F\x98\x80 emoji");
    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "esc.unknown"), "keep q as-is");
    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "plain.key"), "no escapes é raw UTF-8");

    dasher_destroy(ctx);
}

TEST(locale_malformed_degrades) {
    // Truncated / structurally broken files must yield "no translations",
    // never a crash: lookups miss and the engine keeps its English
    // built-ins (missing translation = the documented NULL sentinel).
    const ScopedTempDir tmp;
    const std::string root =
        build_locale_data_dir(tmp, "{ \"broken\": \"unterminated\n{\"deeper\": {\"nested\": \"ignored\"}}");
    dasher_ctx* ctx = dasher_create(root.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx != nullptr);
    ASSERT_EQ(dasher_set_locale(ctx, "zz"), 0);
    CHECK(dasher_get_localized_string(ctx, "no.such.key") == nullptr);
    dasher_destroy(ctx);
}
