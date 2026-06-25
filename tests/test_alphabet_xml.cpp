// Alphabet XML parsing tests: verify alphabet loading, switching, and structure
#include "test_common.h"

TEST(alphabet_default_loaded) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    const char* alph = dasher_get_alphabet_id(ctx);
    printf("  Default alphabet: '%s'\n", alph);
    ASSERT(strlen(alph) > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_count_positive) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_alphabet_count(ctx);
    printf("  Available alphabets: %d\n", count);
    ASSERT(count > 0);

    for (int i = 0; i < count && i < 10; i++) {
        const char* name = dasher_get_alphabet_name(ctx, i);
        printf("  Alphabet %d: '%s'\n", i, name);
        ASSERT(strlen(name) > 0);
    }

    dasher_destroy(ctx);
}

TEST(alphabet_switch_to_english_no_punct) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "English without punctuation");
    const char* loaded = dasher_get_alphabet_id(ctx);
    printf("  Switched to: '%s'\n", loaded);
    ASSERT_STR_EQ(loaded, "English without punctuation");

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  Symbol count: %d\n", sym_count);
    ASSERT(sym_count > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_switch_invalid_falls_back) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    const char* before = dasher_get_alphabet_id(ctx);
    dasher_set_alphabet_id(ctx, "Nonexistent Alphabet XYZ123");
    const char* after = dasher_get_alphabet_id(ctx);
    printf("  Before: '%s', After invalid switch: '%s'\n", before, after);
    ASSERT(strlen(after) > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_symbol_texts_are_nonempty) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    ASSERT(sym_count > 0);

    int nonempty = 0;
    for (int i = 1; i < sym_count && i < 50; i++) {
        char buf[128];
        int rc = dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf));
        if (rc == 0 && strlen(buf) > 0) {
            nonempty++;
        }
    }
    printf("  %d/%d symbols have non-empty text\n", nonempty, sym_count > 50 ? 49 : sym_count - 1);
    ASSERT(nonempty > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_symbol_out_of_range_returns_error) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    char buf[128];
    int rc = dasher_get_alphabet_symbol_text(ctx, sym_count + 100, buf, sizeof(buf));
    ASSERT_EQ(rc, -1);

    rc = dasher_get_alphabet_symbol_text(ctx, -1, buf, sizeof(buf));
    ASSERT_EQ(rc, -1);

    dasher_destroy(ctx);
}

TEST(alphabet_switch_changes_probabilities) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int lbnds1[256], hbnds1[256];
    int n1 = dasher_get_probabilities(ctx, lbnds1, hbnds1, 256);

    dasher_set_alphabet_id(ctx, "English without punctuation");

    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &c, &cc, &s, &sc);

    int lbnds2[256], hbnds2[256];
    int n2 = dasher_get_probabilities(ctx, lbnds2, hbnds2, 256);

    printf("  Before switch: %d children, after: %d children\n", n1, n2);
    ASSERT(n2 > 0);

    bool changed = (n1 != n2);
    if (!changed) {
        for (int i = 0; i < n1; i++) {
            if (lbnds1[i] != lbnds2[i] || hbnds1[i] != hbnds2[i]) {
                changed = true;
                break;
            }
        }
    }
    ASSERT(changed);

    dasher_destroy(ctx);
}

TEST(alphabet_affects_root_child_count) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count1 = dasher_get_root_child_count(ctx);
    printf("  Default alphabet root children: %d\n", count1);

    dasher_set_alphabet_id(ctx, "English without punctuation");
    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &c, &cc, &s, &sc);

    int count2 = dasher_get_root_child_count(ctx);
    printf("  No-punct alphabet root children: %d\n", count2);

    ASSERT(count1 > 0);
    ASSERT(count2 > 0);

    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// v5 → v6 alphabet format support (RFC 0005)
//
// Dasher v5 shipped for 10+ years with a different XML schema. The AlphIO
// parser now accepts both formats. These tests verify the v5 path and guard
// against the v6 regression that PR #28 introduced (space character lost).
// ---------------------------------------------------------------------------

// Regression guard: the v6 space character (label "□", unicode 32) must
// resolve to a literal space in the default alphabet. PR #28 broke this by
// changing the Text default-from-Display logic; this test locks it down.
TEST(alphabet_v6_space_character_resolves_to_space) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    ASSERT(sym_count > 0);

    // Valid symbol indices are 1..sym_count inclusive (index 0 is the sentinel).
    bool found_space = false;
    for (int i = 1; i <= sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) == 0 && strcmp(buf, " ") == 0) {
            found_space = true;
            break;
        }
    }
    printf("  space character present in v6 default alphabet: %s\n", found_space ? "yes" : "no");
    ASSERT(found_space);

    dasher_destroy(ctx);
}

// A v5-format alphabet (root <alphabets>, <s> symbols, <train> child element)
// must load and appear in the alphabet list alongside the bundled v6 files.
TEST(alphabet_v5_format_loads) {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const char* v5xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE alphabets SYSTEM \"alphabet.dtd\">\n"
                        "<alphabets langcode=\"en-GB\">\n"
                        "  <alphabet name=\"V5 Test Alphabet\">\n"
                        "    <orientation type=\"LR\"/>\n"
                        "    <encoding type=\"Western\"/>\n"
                        "    <palette>European/Asian</palette>\n"
                        "    <train>training_english_GB.txt</train>\n"
                        "    <group name=\"Lower case Latin letters\" b=\"0\" visible=\"off\">\n"
                        "      <s d=\"a\" t=\"a\" b=\"10\"/>\n"
                        "      <s d=\"b\" t=\"b\" b=\"11\"/>\n"
                        "      <s d=\"c\" t=\"c\" b=\"12\"/>\n"
                        "    </group>\n"
                        "    <group name=\"Paragraph and space\" b=\"9\">\n"
                        "      <s d=\"\xC2\xB6\" t=\"\xC2\xB6\" b=\"9\"/>\n"
                        "      <s d=\"\xE2\x96\xA1\" t=\" \" b=\"9\"/>\n"
                        "    </group>\n"
                        "  </alphabet>\n"
                        "</alphabets>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.v5test.xml", v5xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    bool found = false;
    int count = dasher_get_alphabet_count(ctx);
    for (int i = 0; i < count; i++) {
        if (strcmp(dasher_get_alphabet_name(ctx, i), "V5 Test Alphabet") == 0) {
            found = true;
            break;
        }
    }
    printf("  v5 alphabet found among %d alphabets: %s\n", count, found ? "yes" : "no");
    ASSERT(found);

    dasher_destroy(ctx);
}

// v5 <s> symbols must produce the correct output text, including the
// display≠text case (box → space) and a multibyte emoji.
TEST(alphabet_v5_symbols_have_correct_text) {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const char* v5xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<alphabets langcode=\"en-GB\">\n"
                        "  <alphabet name=\"V5 Symbol Test\">\n"
                        "    <orientation type=\"LR\"/>\n"
                        "    <train>training_english_GB.txt</train>\n"
                        "    <group name=\"letters\" b=\"0\">\n"
                        "      <s d=\"x\" t=\"x\" b=\"10\"/>\n"
                        "      <s d=\"\xE2\x96\xA1\" t=\" \" b=\"9\"/>\n"
                        "      <s d=\"\xF0\x9F\x98\x80\" t=\"\xF0\x9F\x98\x80\" b=\"125\"/>\n"
                        "    </group>\n"
                        "  </alphabet>\n"
                        "</alphabets>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.v5symbols.xml", v5xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "V5 Symbol Test");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "V5 Symbol Test");

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  v5 alphabet symbol count: %d\n", sym_count);
    ASSERT(sym_count >= 3);

    bool found_x = false, found_space = false, found_emoji = false;
    for (int i = 1; i <= sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) != 0) continue;
        std::string s(buf);
        if (s == "x") found_x = true;
        if (s == " ") found_space = true;
        if (s == "\xF0\x9F\x98\x80") found_emoji = true;
    }
    printf("  x=%d space=%d emoji=%d\n", found_x, found_space, found_emoji);
    ASSERT(found_x);
    ASSERT(found_space);
    ASSERT(found_emoji);

    dasher_destroy(ctx);
}

// v5 metadata in child elements (<train>, <palette>) must be picked up when
// the v6 attributes (trainingFilename, colorsName) are absent. We verify by
// loading the alphabet (which exercises <train>/<palette> parsing) and
// confirming it appears in the list and is switchable.
TEST(alphabet_v5_metadata_from_child_elements) {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const char* v5xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<alphabets langcode=\"en-GB\">\n"
                        "  <alphabet name=\"V5 Meta Test\">\n"
                        "    <orientation type=\"LR\"/>\n"
                        "    <palette>European/Asian</palette>\n"
                        "    <train>training_english_GB.txt</train>\n"
                        "    <group name=\"letters\" b=\"0\">\n"
                        "      <s d=\"q\" t=\"q\" b=\"10\"/>\n"
                        "      <s d=\"w\" t=\"w\" b=\"11\"/>\n"
                        "      <s d=\"e\" t=\"e\" b=\"12\"/>\n"
                        "    </group>\n"
                        "  </alphabet>\n"
                        "</alphabets>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.v5meta.xml", v5xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // The alphabet parsed successfully if it appears in the list — this means
    // the <train> and <palette> child elements were consumed without rejecting
    // the document.
    bool found = false;
    int count = dasher_get_alphabet_count(ctx);
    for (int i = 0; i < count; i++) {
        if (strcmp(dasher_get_alphabet_name(ctx, i), "V5 Meta Test") == 0) {
            found = true;
            break;
        }
    }
    ASSERT(found);

    dasher_set_alphabet_id(ctx, "V5 Meta Test");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "V5 Meta Test");

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  v5 meta alphabet symbols: %d\n", sym_count);
    ASSERT(sym_count > 0);

    dasher_destroy(ctx);
}

// Real v5 format: <space>, <paragraph>, <control> as DIRECT children of
// <alphabet> (not inside a <group>). These must be wrapped in a synthetic
// paragraphSpace group by the parser. <space> → text " ", <paragraph> →
// text "\n" (newline default), <control> → skipped (engine-handled).
TEST(alphabet_v5_special_chars_as_direct_children) {
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    const char* v5xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE alphabets SYSTEM \"alphabet.dtd\">\n"
                        "<alphabets langcode=\"en-GB\">\n"
                        "  <alphabet name=\"V5 Special Chars Test\">\n"
                        "    <orientation type=\"LR\"/>\n"
                        "    <train>training_english_GB.txt</train>\n"
                        "    <space d=\"\xE2\x96\xA1\" t=\" \" b=\"9\"/>\n"
                        "    <paragraph d=\"\xC2\xB6\" b=\"9\"/>\n"
                        "    <control d=\"Control\" t=\"\" b=\"8\"/>\n"
                        "    <group name=\"Letters\" b=\"0\">\n"
                        "      <s d=\"a\" t=\"a\" b=\"10\"/>\n"
                        "      <s d=\"b\" t=\"b\" b=\"11\"/>\n"
                        "    </group>\n"
                        "  </alphabet>\n"
                        "</alphabets>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.v5special.xml", v5xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "V5 Special Chars Test");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "V5 Special Chars Test");

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  v5 special-chars alphabet symbols: %d\n", sym_count);
    // 2 letters + space + paragraph = 4 (control is skipped)
    ASSERT(sym_count >= 4);

    // Scan all symbols for the expected text values.
    bool found_letter_a = false, found_space = false, found_newline = false;
    for (int i = 1; i <= sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) != 0) continue;
        std::string s(buf);
        if (s == "a") found_letter_a = true;
        if (s == " ") found_space = true;
        if (s == "\n") found_newline = true;
    }
    printf("  letter_a=%d space=%d newline=%d\n", found_letter_a, found_space, found_newline);
    ASSERT(found_letter_a);
    ASSERT(found_space);
    ASSERT(found_newline);

    dasher_destroy(ctx);
}
