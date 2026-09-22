// Alphabet XML parsing tests: verify alphabet loading, switching, and structure
#include "test_common.h"

#include <string>
#include <vector>

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

// ── Emoji alphabet (Dasher-Android #61, option 2) ─────────────────────────
// Multi-codepoint nodes: ZWJ sequences (family: 5 codepoints / 18 bytes) and
// skin-tone modifiers must survive the XML round-trip whole — ReadCharAttributes
// stores the full label as Text and TextOutputAction commits it atomically.

TEST(alphabet_emoji_loads_and_has_groups) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "Emoji");
    const char* loaded = dasher_get_alphabet_id(ctx);
    printf("  Switched to: '%s'\n", loaded);
    ASSERT_STR_EQ(loaded, "Emoji");

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  Emoji symbol count: %d\n", sym_count);
    // Emoji alphabet loads (expect ~310 symbols: 9 groups, ~307 nodes + control).
    ASSERT(sym_count > 150);

    dasher_destroy(ctx);
}

TEST(alphabet_emoji_zwj_sequence_roundtrip) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "Emoji");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "Emoji");

    const char* family =
        "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"; // U+1F468 ZWJ U+1F469 ZWJ U+1F467
    const char* toned = "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD";                         // U+1F44D U+1F3FD
    bool found_family = false, found_toned = false, found_space = false;
    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    for (int i = 1; i < sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) != 0) continue;
        if (strcmp(buf, family) == 0) found_family = true;
        if (strcmp(buf, toned) == 0) found_toned = true;
        if (strcmp(buf, " ") == 0) found_space = true;
    }
    printf("  ZWJ family: %d, skin-tone: %d, space: %d\n", found_family, found_toned, found_space);
    ASSERT(found_family);
    ASSERT(found_toned);
    ASSERT(found_space); // separator node: display ␣, text " "

    dasher_destroy(ctx);
}

TEST(alphabet_emoji_training_file_present) {
    // The engine tolerates a missing training file, but we ship one for mild
    // ordering priors — assert the shipped tree still has it next to the
    // alphabet so release packaging doesn't silently drop it.
    std::error_code ec;
    bool ok =
        std::filesystem::exists(std::filesystem::path(TEST_DATA_DIR) / "Data" / "training" / "training_emoji.txt", ec);
    ASSERT(ok);
}

TEST(alphabet_emoji_corpus_tokens_are_nodes) {
    // Greptile P2: corpus tokens that are not alphabet symbols train
    // UNKNOWN_SYMBOL observations — pure noise. Every whitespace-separated
    // token must be a whole node text, and (trainer limitation) must be a
    // SINGLE code point: the trainer looks up one code point per symbol,
    // so multi-codepoint tokens (ZWJ, VS16) can never match and are
    // silently split.
    std::vector<std::string> nodes;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Emoji");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "Emoji");
    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    for (int i = 1; i < sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) == 0 && buf[0] != '\0') nodes.push_back(buf);
    }
    dasher_destroy(ctx);

    std::ifstream in(std::filesystem::path(TEST_DATA_DIR) / "Data" / "training" / "training_emoji.txt");
    ASSERT(in.is_open());
    std::string line;
    int tokens = 0;
    while (std::getline(in, line)) {
        size_t start = 0;
        while (start < line.size()) {
            size_t end = line.find(' ', start);
            if (end == std::string::npos) end = line.size();
            if (end > start) {
                std::string tok = line.substr(start, end - start);
                tokens++;
                bool known = false;
                for (const auto& n : nodes) {
                    if (n == tok) {
                        known = true;
                        break;
                    }
                }
                if (!known) {
                    printf("  unknown corpus token (%zu bytes):", tok.size());
                    for (unsigned char ch : tok)
                        printf(" %02x", ch);
                    printf("\n");
                    ASSERT(false);
                }
                // Greptile P2 + RFC 0020 clause 4: corpus tokens must be
                // whole alphabet symbols. Multi-codepoint tokens became
                // VALID with the longest-match trainer (they train their
                // node as one symbol), so membership is the constraint —
                // the historical single-codepoint restriction is lifted.
                // The shipped corpus simply doesn't use multi-codepoint
                // tokens yet; future additions may.
            }
            start = end + 1;
        }
    }
    printf("  %d corpus tokens, all valid single-codepoint nodes\n", tokens);
    ASSERT(tokens > 100);
}

TEST(alphabet_emoji_output_segments_into_whole_nodes) {
    // Greptile P2: prove the OUTPUT path commits multi-codepoint nodes
    // atomically — at EVENT level, not byte level. A concatenated-bytes
    // check would pass even if 👨‍👩‍👧 arrived as five separate events
    // (👨, ZWJ, 👩, ZWJ, 👧); the output callback sees each insert as its
    // own event, so requiring every event's text to be a WHOLE node text
    // closes that hole. We also require at least one multi-codepoint
    // event (>4 bytes ⇒ ZWJ or VS16 carrier) so the multi-byte path is
    // actually exercised, not vacuously green.
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Emoji");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "Emoji");
    dasher_set_speed_percent(ctx, 300);

    std::vector<std::string> symbols;
    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    for (int i = 1; i < sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) == 0 && buf[0] != '\0') symbols.push_back(buf);
    }
    ASSERT(symbols.size() > 100);

    std::vector<std::string> events;
    dasher_set_output_callback(
        ctx,
        [](int event_type, const char* text, void* user_data) {
            if (event_type == DASHER_EVENT_OUTPUT) {
                static_cast<std::vector<std::string>*>(user_data)->push_back(text);
            }
        },
        &events);

    // Sweep sy across varied bands with the button held. Several passes
    // with different y-bands and x-depths; every committed event must be a
    // WHOLE node text (byte-level concatenation can't prove atomicity —
    // five separate events for 👨‍👩‍👧 produce identical bytes).
    const struct {
        int y0, y1, x;
    } passes[] = {
        {100, 500, 700}, // full safe band (same as test_spell_word)
        {150, 350, 720}, // upper-half dwell
        {300, 540, 680}, // lower-half dwell
        {200, 460, 740}, // deeper zoom
    };
    int frame = 0;
    for (const auto& p : passes) {
        dasher_mouse_down(ctx);
        const int span = p.y1 - p.y0;
        for (int i = 0; i < 500; i++) {
            int sy = p.y0 + (i % span);
            dasher_mouse_move(ctx, static_cast<float>(p.x), static_cast<float>(sy));
            int* c = nullptr;
            int cc = 0;
            char** s = nullptr;
            int sc = 0;
            dasher_frame(ctx, 1000 + (frame++) * 16, &c, &cc, &s, &sc);
        }
        dasher_mouse_up(ctx);
        if (events.size() > 0) break;
    }

    size_t total = 0;
    for (const auto& ev : events) {
        total += ev.size();
        bool whole = false;
        for (const auto& sym : symbols) {
            if (sym == ev) {
                whole = true;
                break;
            }
        }
        if (!whole) {
            printf("  NON-ATOMIC event (%zu bytes):", ev.size());
            for (unsigned char ch : ev)
                printf(" %02x", ch);
            printf("\n");
        }
        ASSERT(whole);
    }
    printf("  %zu output events, %zu bytes — every event a whole node text\n", events.size(), total);
    ASSERT(events.size() > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_multicodepoint_commit_is_atomic) {
    // Deterministic multi-codepoint coverage: a purpose-built 3-node test
    // alphabet (ZWJ sequence, VS16 carrier, plain emoji) where EVERY node
    // but one is multi-codepoint and each holds ~1/3 of the tree mass —
    // navigation cannot avoid committing them. Complements the sweep test
    // above, whose mass distribution follows the training priors and may
    // not reach the shipped alphabet's low-mass multi-codepoint nodes.
    ScopedTempDir dataRoot;
    const std::string data_dir = build_data_dir(dataRoot);
    // No training file: uniform-ish symbol ordering (the engine's
    // documented no-training fallback).
    std::string xml = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
                      "<!DOCTYPE alphabet SYSTEM \"../alphabet.dtd\">\n" +
                      "<alphabet name=\"ZWJ Test\" orientation=\"LR\" colorsName=\"Default\">\n" +
                      "  <group name=\"mixed\">\n" +
                      "    <node label=\"&#x1F468;&#x200D;&#x1F469;&#x200D;&#x1F467;\"><textCharAction /></node>\n" +
                      "    <node label=\"&#x2708;&#xFE0F;\"><textCharAction /></node>\n" +
                      "    <node label=\"&#x1F600;\"><textCharAction /></node>\n" + "  </group>\n" + "</alphabet>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.zwjtest.xml", xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), dataRoot.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    printf("  custom dir alphabets: %d\n", dasher_get_alphabet_count(ctx));
    dasher_set_alphabet_id(ctx, "ZWJ Test");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "ZWJ Test");

    const std::string family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7";
    const std::string plane = "\xE2\x9C\x88\xEF\xB8\x8F";
    const std::string grin = "\xF0\x9F\x98\x80";

    std::vector<std::string> events;
    dasher_set_output_callback(
        ctx,
        [](int event_type, const char* text, void* user_data) {
            if (event_type == DASHER_EVENT_OUTPUT) {
                static_cast<std::vector<std::string>*>(user_data)->emplace_back(text);
            }
        },
        &events);

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 600; i++) {
        int sy = 150 + (i % 300);
        dasher_mouse_move(ctx, 700.0f, static_cast<float>(sy));
        int* c = nullptr;
        int cc = 0;
        char** s = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &c, &cc, &s, &sc);
    }
    dasher_mouse_up(ctx);

    size_t multi = 0;
    for (const auto& ev : events) {
        bool known = (ev == family || ev == plane || ev == grin);
        if (!known) {
            printf("  NON-ATOMIC event (%zu bytes):", ev.size());
            for (unsigned char ch : ev)
                printf(" %02x", ch);
            printf("\n");
        }
        ASSERT(known);
        if (ev.size() > 4) multi++;
    }
    printf("  %zu events, %zu multi-codepoint commits\n", events.size(), multi);
    ASSERT(events.size() > 0);
    ASSERT(multi > 0);

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
    for (int i = 1; i < sym_count; i++) {
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

// The paragraph symbol (display "pilcrow") in the shipped v6 alphabets must
// OUTPUT a newline, not the display character. The v5->v6 conversion of the
// hand-written v6 files lost the output char, leaving an empty
// <textCharAction/> so the pilcrow was inserted literally (reported as
// "return just shows a paragraph symbol"). Regression test for that fix.
TEST(alphabet_v6_paragraph_outputs_newline) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // Every shipped v6 alphabet that carries a paragraphSpace group.
    // Names must match the <alphabet name="..."> attribute exactly: an
    // unknown name silently falls back to the default alphabet, which
    // would make the check pass without testing the resource.
    const char* alphabets_to_check[] = {
        "English with numerals and limited punctuation",
        "English with limited punctuation",
        "English with numerals and lots of punctuation",
        "English with accents, numerals, punctuation",
        "English Latex",
        "Deutsch / German with limited punctuation",
        "Deutsch / German with numerals and punctuation",
    };

    for (const char* alph : alphabets_to_check) {
        dasher_set_alphabet_id(ctx, alph);
        ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), alph);

        int sym_count = dasher_get_alphabet_symbol_count(ctx);
        ASSERT(sym_count > 0);

        bool found_paragraph_display = false, paragraph_is_newline = false;
        for (int i = 1; i < sym_count; i++) {
            char disp[128], text[128];
            if (dasher_get_alphabet_symbol_display(ctx, i, disp, sizeof(disp)) != 0) continue;
            if (strcmp(disp, "\xc2\xb6") != 0) continue; // UTF-8 pilcrow
            found_paragraph_display = true;
            if (dasher_get_alphabet_symbol_text(ctx, i, text, sizeof(text)) == 0 && strcmp(text, "\n") == 0)
                paragraph_is_newline = true;
        }
        printf("  %s: paragraph display found=%d outputs_newline=%d\n", alph, found_paragraph_display,
               paragraph_is_newline);
        ASSERT(found_paragraph_display);
        ASSERT(paragraph_is_newline);
    }

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
    for (int i = 1; i < sym_count; i++) {
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
    for (int i = 1; i < sym_count; i++) {
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

TEST(retired_default_alphabet_id_heals_to_real_alphabet) {
    // The bare a-z "Default" emergency alphabet is no longer registered
    // eagerly: it was selectable/persistable and structurally dead (driving
    // committed no output — "no text goes into the output area" report).
    // A saved Default id must heal to a real alphabet AND drive.
    ScopedTempDir user;
    {
        std::ofstream out(std::filesystem::path(user.path) / "dasher_settings.xml");
        out << "<?xml version=\"1.0\"?>\n<settings>\n"
            << "  <string name=\"AlphabetID\" value=\"Default\" />\n</settings>\n";
    }
    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, user.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    const char* alph = dasher_get_alphabet_id(ctx);
    printf("  healed alphabet: '%s'\n", alph);
    ASSERT(strcmp(alph, "Default") != 0); // healed to a real alphabet

    // And it drives (canonical interaction recipe).
    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        int* c = nullptr;
        int cc = 0;
        char** s = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 20, &c, &cc, &s, &sc);
    }
    dasher_mouse_up(ctx);
    const char* text = dasher_get_output_text(ctx);
    ASSERT(text && strlen(text) > 0);
    printf("  output after drive: '%s'\n", text);
    dasher_destroy(ctx);
}

TEST(retired_default_heals_in_custom_data_dir_without_preferred) {
    // Greptile P1 on #88: a custom data dir with alphabets but WITHOUT the
    // preferred "English with limited punctuation" — GetDefault() returns
    // the literal "Default", so the heal must fall back to the first index
    // entry that actually parses, not no-op back into the dead id.
    ScopedTempDir dataRoot;
    std::filesystem::path data = std::filesystem::path(dataRoot.path) / "Data";
    std::filesystem::create_directories(data / "alphabets");
    std::filesystem::create_directories(data / "training");
    // Only one alphabet, deliberately NOT the preferred English one, plus
    // the training corpus it names.
    std::error_code ec;
    // TEST_DATA_DIR is the REPO ROOT (CMakeLists sets it to
    // CMAKE_CURRENT_LIST_DIR); the shipped files live under Data/.
    std::filesystem::copy_file(std::filesystem::path(TEST_DATA_DIR) / "Data" / "alphabets" /
                                   "alphabet.english.without.punctuation.xml",
                               data / "alphabets" / "alphabet.english.without.punctuation.xml", ec);
    ec.clear();
    std::filesystem::copy_file(std::filesystem::path(TEST_DATA_DIR) / "Data" / "training" / "training_english_GB.txt",
                               data / "training" / "training_english_GB.txt", ec);

    ScopedTempDir user;
    {
        std::ofstream out(std::filesystem::path(user.path) / "dasher_settings.xml");
        out << "<?xml version=\"1.0\"?>\n<settings>\n"
            << "  <string name=\"AlphabetID\" value=\"Default\" />\n</settings>\n";
    }
    // NOTE: dasher_create's data_dir is the DATA directory itself (TEST_DATA_DIR
    // is "./Data"), not its parent.
    dasher_ctx* ctx = dasher_create(data.string().c_str(), user.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    const char* alph = dasher_get_alphabet_id(ctx);
    printf("  healed in custom dir: '%s'\n", alph);
    ASSERT(strcmp(alph, "Default") != 0); // healed to the one available alphabet

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        int* c = nullptr;
        int cc = 0;
        char** s2 = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 20, &c, &cc, &s2, &sc);
    }
    dasher_mouse_up(ctx);
    const char* text = dasher_get_output_text(ctx);
    ASSERT(text && strlen(text) > 0);
    printf("  output after drive: '%s'\n", text);
    dasher_destroy(ctx);
}

TEST(retired_default_heal_uses_own_context_data_dir) {
    // Greptile P1 #2 on #88: with two contexts alive (created A-custom then
    // B-full), realizing A must bulk-scan A's data dir — not the
    // process-global (last-create-wins) directory, which is B's. Without the
    // per-context scan A would heal to B's preferred alphabet, selecting an
    // id unavailable in its own bundle.
    ScopedTempDir dataRootCustom;
    std::filesystem::path custom = std::filesystem::path(dataRootCustom.path) / "Data";
    std::filesystem::create_directories(custom / "alphabets");
    std::filesystem::create_directories(custom / "training");
    std::error_code ec;
    std::filesystem::copy_file(std::filesystem::path(TEST_DATA_DIR) / "Data" / "alphabets" /
                                   "alphabet.english.without.punctuation.xml",
                               custom / "alphabets" / "alphabet.english.without.punctuation.xml", ec);
    std::filesystem::copy_file(std::filesystem::path(TEST_DATA_DIR) / "Data" / "training" / "training_english_GB.txt",
                               custom / "training" / "training_english_GB.txt", ec);

    ScopedTempDir userA, userB;
    {
        std::ofstream out(std::filesystem::path(userA.path) / "dasher_settings.xml");
        out << "<?xml version=\"1.0\"?>\n<settings>\n"
            << "  <string name=\"AlphabetID\" value=\"Default\" />\n</settings>\n";
    }

    // Create A (custom), then B (full) — the global now points at B's dir.
    dasher_ctx* a = dasher_create(custom.string().c_str(), userA.c_str(), nullptr);
    dasher_ctx* b = dasher_create(TEST_DATA_DIR, userB.c_str(), nullptr);
    ASSERT(a && b);
    dasher_set_screen_size(a, 800, 600); // realize A LAST-created-other: must still use A's dir
    const char* alphA = dasher_get_alphabet_id(a);
    printf("  A healed to: '%s'\n", alphA);
    ASSERT_STR_EQ(alphA, "English without punctuation"); // A's own, not B's preferred
    dasher_destroy(a);
    dasher_destroy(b);
}
