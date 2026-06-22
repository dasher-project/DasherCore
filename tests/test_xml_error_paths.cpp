// test_xml_error_paths.cpp
//
// CHARACTERIZATION TESTS for XML parser error handling. Before this file,
// every test used the well-formed bundled files in Data/. There was zero
// coverage of malformed input — yet the parsers (AlphIO, ColorIO,
// XmlSettingsStore, AbstractXMLParser) handle user-editable files in
// production and must not crash on bad input.
//
// Strategy: write malformed XML to temp files in a ScopedTempDir, point a
// fresh dasher_ctx at them via a hand-crafted Data/ directory, and assert
// that the engine either ignores the bad file or reports a message — but
// never crashes or leaves the context unusable.
//
// Note: there is no public C API to inject a custom alphabet XML directly.
// The engine discovers files via FileUtils::ScanFiles("alphabet.*.xml").
// We create a Data/ directory in the test temp dir, copy in the training
// data, and add our malformed file alongside.

#include "test_common.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

// build_data_dir and write_data_file are provided by test_common.h.

// ---------------------------------------------------------------------------
// Foundational: the build_data_dir helper works
// ---------------------------------------------------------------------------

TEST_CASE("xml/helper builds a working Data dir") {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);
    REQUIRE(data_dir.size() > 0);

    // A context created with this data dir must realize successfully.
    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* alphabet = dasher_get_alphabet_id(ctx);
    REQUIRE(alphabet != nullptr);
    CHECK(std::string(alphabet).size() > 0);

    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// Malformed alphabet XML: the engine must not crash, and must fall back
// to a working alphabet (whichever valid one loaded).
// ---------------------------------------------------------------------------

TEST_CASE("xml/truncated alphabet file is ignored") {
    // CHARACTERIZATION: write an alphabet.*.xml file that is syntactically
    // invalid (truncated XML). The engine must skip it without crashing.
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const std::string truncated =
        "<?xml version=\"1.0\"?>\n"
        "<alphabet name=\"TruncatedTest\">\n"
        "  <orientation type=\"LR\"/>\n"
        "  <train>dasher_training_en_GB.txt</train>\n"
        "  <spacebar output=\" \"/>\n";  // no closing </alphabet>

    REQUIRE(write_data_file(data_dir, "alphabets",
                            "alphabet.truncated_test.xml", truncated));

    // Creating a context with this file present must not crash.
    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // The engine must have loaded SOME alphabet (not the truncated one).
    const char* alphabet = dasher_get_alphabet_id(ctx);
    REQUIRE(alphabet != nullptr);
    // The truncated alphabet name must not appear (it was rejected).
    CHECK(std::string(alphabet) != "TruncatedTest");

    // Switching to the bad alphabet by name must fail safely.
    dasher_set_alphabet_id(ctx, "TruncatedTest");
    // Engine falls back to a valid alphabet; we don't care which.
    const char* fallback = dasher_get_alphabet_id(ctx);
    CHECK(fallback != nullptr);

    dasher_destroy(ctx);
}

TEST_CASE("xml/alphabet with binary garbage is ignored") {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    // Pure binary content — pugixml must reject this cleanly.
    std::string garbage;
    garbage.push_back('\0');
    garbage.push_back('\x01');
    garbage.push_back('\xFF');
    garbage.push_back('\xFE');
    garbage.append("not xml at all");

    REQUIRE(write_data_file(data_dir, "alphabets",
                            "alphabet.garbage_test.xml", garbage));

    // Engine still realizes (other valid alphabets load).
    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    CHECK(std::string(dasher_get_alphabet_id(ctx)) != "garbage_test");
    dasher_destroy(ctx);
}

TEST_CASE("xml/empty alphabet file is ignored") {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    REQUIRE(write_data_file(data_dir, "alphabets",
                            "alphabet.empty_test.xml", ""));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    // Empty file -> not a valid alphabet -> must not be selectable.
    CHECK(std::string(dasher_get_alphabet_id(ctx)) != "empty_test");
    dasher_destroy(ctx);
}

TEST_CASE("xml/alphabet with valid XML but wrong schema is ignored") {
    // Well-formed XML, but the root element is not <alphabet> — must be
    // ignored by AlphIO::Parse without affecting other alphabets.
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const std::string wrong_schema =
        "<?xml version=\"1.0\"?>\n"
        "<wrong_root>\n"
        "  <some_tag>value</some_tag>\n"
        "</wrong_root>\n";

    REQUIRE(write_data_file(data_dir, "alphabets",
                            "alphabet.wrong_schema.xml", wrong_schema));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    CHECK(std::string(dasher_get_alphabet_id(ctx)) != "wrong_schema");
    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// Malformed colour/palette XML
// ---------------------------------------------------------------------------

TEST_CASE("xml/truncated colour file is ignored") {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const std::string truncated =
        "<?xml version=\"1.0\"?>\n"
        "<colors name=\"TruncatedPalette\">\n"
        "  <palette name=\"main\">\n"
        "    <entry colour=\"#000000\"";  // truncated

    REQUIRE(write_data_file(data_dir, "colours",
                            "colours.truncated.xml", truncated));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Engine still realizes; "TruncatedPalette" must not be available.
    const int sp_palette = dasher_find_parameter_key("SP_COLOUR_ID");
    if (sp_palette > 0) {
        // The bad palette must not appear in the palette list.
        const char* names[64];
        int n = dasher_get_palette_count(ctx);
        // (We don't assert >0 — palette availability is data-dependent.)
        for (int i = 0; i < n && i < 64; ++i) {
            names[i] = dasher_get_palette_name(ctx, i);
        }
        for (int i = 0; i < n && i < 64; ++i) {
            CHECK(std::string(names[i]) != "TruncatedPalette");
        }
    }

    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// Settings XML: bad file must not corrupt the in-memory store
// ---------------------------------------------------------------------------

TEST_CASE("xml/malformed settings load does not crash") {
    // The settings file lives at user_dir + "/dasher_settings.xml" (or
    // similar — see XmlSettingsStore::ParseFile). Write malformed XML
    // there; creating a context (which loads settings) must not crash and
    // the engine must realize with defaults.
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    // Write a malformed settings file into the user dir.
    const std::string bad_settings =
        "<?xml version=\"1.0\"?>\n"
        "<settings>\n"
        "  <item name=\"LP_MAX_BITRATE\">not_a_number";
    std::ofstream out(std::string(tmp.path) + "/dasher_settings.xml");
    out << bad_settings;
    out.close();

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Long parameters should have valid defaults despite the bad file.
    const int lp_bitrate = dasher_find_parameter_key("LP_MAX_BITRATE");
    if (lp_bitrate > 0) {
        long v = dasher_get_long_parameter(ctx, lp_bitrate);
        CHECK(v > 0);
    }

    dasher_destroy(ctx);
}

TEST_CASE("xml/dash-only settings file does not corrupt state") {
    // An empty or comment-only settings file must be a no-op. The engine
    // uses parameter defaults — which for LP_MAX_BITRATE is 80 (see
    // Parameters.cpp:166), giving a speed_percent of 80/160*100 = 50.
    // (The README's "100 = default speed" is documentation drift; the
    // actual default returns 50 via dasher_get_speed_percent.)
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const std::string empty_settings =
        "<?xml version=\"1.0\"?>\n"
        "<!-- intentionally empty -->\n";

    std::ofstream out(std::string(tmp.path) + "/dasher_settings.xml");
    out << empty_settings;
    out.close();

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Documented actual default.
    CHECK(dasher_get_speed_percent(ctx) == 50);
    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// Concurrent bad files: multiple bad files at once must all be skipped
// ---------------------------------------------------------------------------

TEST_CASE("xml/multiple bad files at once are all skipped") {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    REQUIRE(write_data_file(data_dir, "alphabets", "alphabet.bad1.xml", ""));
    REQUIRE(write_data_file(data_dir, "alphabets", "alphabet.bad2.xml", "garbage"));
    REQUIRE(write_data_file(data_dir, "alphabets", "alphabet.bad3.xml",
                             "<?xml version=\"1.0\"?><not_alphabet/>"));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Engine still realizes and has SOME alphabet loaded.
    CHECK(std::string(dasher_get_alphabet_id(ctx)).size() > 0);

    // At least one alphabet must be selectable.
    int n = dasher_get_alphabet_count(ctx);
    CHECK(n > 0);

    dasher_destroy(ctx);
}
