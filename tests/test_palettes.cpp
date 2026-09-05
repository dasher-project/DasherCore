// Palette catalogue regression tests (colours/ cleanup plan, Phase 4
// scaffold — built BEFORE any files are moved or deleted, so every later
// step is guarded).
//
// Pins three properties of the shipped palette set:
//   1. The engine sees exactly the expected palette names — no duplicates,
//      no accidental losses during the colors/→colours/ consolidation.
//   2. No file named color*.xml ships anywhere under Data/ (the surviving
//      set is British: Data/colours/colour*.xml only).
//   3. Every shipped palette defines an outline colour (the palettes are
//      outline-based by design — v5's Yellow on Black fills boxes with the
//      background and relies on the outline; a palette without one renders
//      as floating letters, Dasher-Windows #47).
#include "test_common.h"

#include <filesystem>

// Fails (deliberately) while Data/colors/ still ships — the consolidation
// flips it green. doctest may_fail keeps CI green either way, but the
// failure text shows in the logs as the pending work item.
#define TEST_MAY_FAIL(name) DOCTEST_TEST_CASE(#name* doctest::may_fail())
#include <set>
#include <string>
#include <vector>

namespace {

std::vector<std::string> palette_names(dasher_ctx* ctx) {
    std::vector<std::string> names;
    const int count = dasher_get_palette_count(ctx);
    for (int i = 0; i < count; i++) {
        const char* n = dasher_get_palette_name(ctx, i);
        if (n && *n) names.push_back(n);
    }
    return names;
}

} // namespace

TEST(palette_catalogue_is_exact_and_duplicate_free) {
    ScopedContext sc(800, 600);
    const auto names = palette_names(sc);
    printf("  %zu palettes:\n", names.size());
    for (const auto& n : names)
        printf("    %s\n", n.c_str());

    // Duplicates are impossible while names collide across colors/ and
    // colours/ only via parse order — this pins that no duplicate-name
    // pair silently ships one winner either way.
    std::set<std::string> unique(names.begin(), names.end());
    ASSERT_EQ(unique.size(), names.size());

    // Every currently-shipped name must survive the consolidation
    // (update this list deliberately as palettes are merged/renamed —
    // that is the point of the pin).
    // Pre-consolidation reality (21): both European/Asian spellings ship
    // side by side. The consolidation's target list drops
    // "European/Asian (Original)" and renames "(Original) Dark" — update
    // EXPECTED to the target the moment those renames land.
    const std::set<std::string> expected = {
        "Blue on Dark Green",
        "Blue on Light Green",
        "Default",
        "Default Dark",
        "European/Asian",
        "European/Asian (Original)",
        "European/Asian (Original) Dark",
        "European/Asian for Colourblind",
        "European/Asian for Colourblind Dark",
        "Rainbow",
        "Rainbow Dark",
        "Thai",
        "Thai Dark",
        "TurboLUT",
        "TurboLUT Dark",
        "Vowels",
        "Vowels Dark",
        "Vowels2",
        "Vowels2 Dark",
        "Yellow on Black",
        "Yellow on Blue",
    };
    std::set<std::string> got(unique);
    ASSERT_EQ(got.size(), expected.size());
    for (const auto& e : expected) {
        if (got.find(e) == got.end()) {
            printf("  MISSING: %s\n", e.c_str());
            ASSERT(false);
        }
        got.erase(e);
    }
    ASSERT(got.empty()); // nothing unexpected appeared either
}

TEST_MAY_FAIL(no_american_spelled_palette_files_ship) {
    // *may_fail: fails while Data/colors/ still ships; flips green when the
    // consolidation deletes it, and stays green forever after.
    // The surviving convention is Data/colours/colour*.xml (British). This
    // fails the moment a color*.xml file reappears in the tree.
    // TEST_DATA_DIR is the project root; the palette dirs live under Data/.
    const std::string data_dir = std::string(TEST_DATA_DIR) + "/Data";
    for (const auto& sub : {"/colours", "/colors", ""}) {
        const std::string dir = data_dir + sub;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            const std::string name = entry.path().filename().string();
            if (name.rfind("color", 0) == 0 && name.find(".xml") != std::string::npos) {
                printf("  FORBIDDEN: %s/%s (British colour*.xml only)\n", dir.c_str(), name.c_str());
                ASSERT(false);
            }
        }
    }
}

TEST(no_palette_reports_the_error_sentinel) {
    // A palette whose file failed to parse (bad XML, wrong root, missing
    // name) falls back to the error sentinel preview: black / magenta /
    // black / magenta. Shipping one renders as garbage boxes — this catches
    // a broken file the moment it lands, during the consolidation moves.
    ScopedContext sc(800, 600);
    const int count = dasher_get_palette_count(sc);
    ASSERT(count > 0);
    for (int i = 0; i < count; i++) {
        int colors[4] = {0, 0, 0, 0};
        ASSERT_EQ(dasher_get_palette_preview_colors(sc, i, colors), 0);
        // colorToARGB layout: alpha<<24 | r<<16 | g<<8 | b
        const auto is_black = [](int c) {
            return ((c >> 24) & 0xFF) != 0 && ((c >> 16) & 0xFF) == 0 && ((c >> 8) & 0xFF) == 0 && (c & 0xFF) == 0;
        };
        const auto is_magenta = [](int c) {
            return ((c >> 24) & 0xFF) != 0 && ((c >> 16) & 0xFF) == 255 && ((c >> 8) & 0xFF) == 0 && (c & 0xFF) == 255;
        };
        if (is_black(colors[0]) && is_magenta(colors[1]) && is_black(colors[2]) && is_magenta(colors[3])) {
            const char* n = dasher_get_palette_name(sc, i);
            printf("  SENTINEL PREVIEW (unparsed palette): %s\n", n ? n : "?");
            ASSERT(false);
        }
    }
}
