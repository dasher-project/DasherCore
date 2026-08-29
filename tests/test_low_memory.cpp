// Low-memory mode tests: verify reduced memory footprint
#include "test_common.h"
TEST(low_memory_alphabet_count) {
    // Lazy alphabet loading (all modes): the alphabet MENU lists every
    // known alphabet via the cheap name index; only the selected alphabet
    // is fully parsed. The old "count <= 2" asserted the menu reflected
    // the parsed set — that semantic is gone. What stays testable from
    // the CAPI: the full inventory is listed, identically in low-memory
    // and normal modes (parsing differs, the menu does not).
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_low_memory_mode(ctx, 1);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_alphabet_count(ctx);
    printf("  Low-memory alphabet count: %d\n", count);
    ASSERT(count > 100); // full inventory via the name index

    dasher_destroy(ctx);

    // Compare with normal mode — identical menu, parsing differs
    ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int normal_count = dasher_get_alphabet_count(ctx);
    printf("  Normal alphabet count: %d\n", normal_count);
    ASSERT_EQ(normal_count, count);

    dasher_destroy(ctx);
}

TEST(low_memory_text_output) {
    // Low-memory mode should still produce text
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_low_memory_mode(ctx, 1);
    dasher_set_screen_size(ctx, 800, 600);

    // Verify the alphabet is functional
    const char* alph_id = dasher_get_alphabet_id(ctx);
    ASSERT(alph_id != nullptr);
    ASSERT(strlen(alph_id) > 0);
    printf("  Alphabet: '%s'\n", alph_id);

    // Try interaction
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 100; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000, 20);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    printf("  Output: '%s'\n", text);
    ASSERT(strlen(text) > 0);

    dasher_destroy(ctx);
}

TEST(low_memory_alphabet_switch) {
    // In low-memory mode, switching alphabets should lazy-load the new one
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_low_memory_mode(ctx, 1);
    dasher_set_screen_size(ctx, 800, 600);

    int initial_count = dasher_get_alphabet_count(ctx);
    printf("  Initial alphabet count: %d\n", initial_count);

    // Try switching to a different alphabet
    const char* alphabets[] = {"English without punctuation", "English lower case", "English with limited punctuation"};

    for (int i = 0; i < 3; i++) {
        dasher_set_alphabet_id(ctx, alphabets[i]);
        const char* current = dasher_get_alphabet_id(ctx);
        printf("  Set '%s' -> got '%s'\n", alphabets[i], current ? current : "(null)");
        // After setting, the alphabet should be loaded
        int count = dasher_get_alphabet_count(ctx);
        printf("  Alphabet count after switch: %d\n", count);
    }

    // Should still produce text after switching
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 50; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000, 20);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);

    dasher_destroy(ctx);
}

TEST(alphabet_switch_to_duplicate_symbol_file) {
    // Regression: 132 shipped alphabet files define some symbols twice
    // (autoconverted case groups on caseless scripts, shared Mandarin tone
    // outputs, ...). CAlphabetMap::Add used to DASHER_ASSERT on the second
    // definition — SIGABRT the moment one of these was selected. Switching
    // to the worst offender in the shipped corpus (every Arabic letter
    // twice: 36 duplicate symbols) must load and run.
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Arabic (WorldAlphabets)");

    const char* current = dasher_get_alphabet_id(ctx);
    printf("  After switch: '%s'\n", current ? current : "(null)");
    ASSERT(current != nullptr);
    ASSERT(std::string(current) == "Arabic (WorldAlphabets)");

    // The engine must produce frames on the duplicate-containing alphabet.
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 20; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000, 20);
    }
    dasher_mouse_up(ctx);

    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);
    ASSERT(commands != nullptr);
    ASSERT(cmd_count > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_index_duplicate_ids_prefer_maintained) {
    // Regression (PR #68 review): when two files declare the same AlphID,
    // the index previously kept whichever the filesystem traversal visited
    // last — unspecified. Now the resolution is deterministic and prefers
    // the authoritative definition: maintained root file over the
    // autoConverted export over the oldAlphabets v5 original. Both orders
    // are created here explicitly so the test cannot depend on readdir luck.
    // ScopedTempDir: stale-dir removal + RAII cleanup (see the scanner
    // edge-case test for the flakiness this prevents).
    ScopedTempDir dataDir, userDir;
    char sub[512];
    snprintf(sub, sizeof(sub), "%s/oldAlphabets", dataDir.c_str());
    std::error_code ec;
    std::filesystem::create_directories(sub, ec);

    // Maintained definition: 30 letters — 31 symbols, deliberately unlike
    // the builtin Default alphabet (26 letters / 27 symbols) so the count
    // assertion below cannot pass via a silent Default fallback.
    FILE* f = fopen((std::string(dataDir) + "/alphabet.dup.xml").c_str(), "w");
    ASSERT(f != nullptr);
    fputs("<alphabet name='Dup Test'><group name='Letters'>\n", f);
    for (char c = 'a'; c <= 'z'; c++)
        fprintf(f, "<node label='%c'><textCharAction /></node>\n", c);
    fputs("<node label='&#x00e9;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00e8;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00ea;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00eb;'><textCharAction /></node>\n", f);
    fputs("</group></alphabet>\n", f);
    fclose(f);

    // Legacy duplicate: same AlphID, only 3 letters.
    f = fopen((std::string(sub) + "/alphabet.dup.legacy.xml").c_str(), "w");
    ASSERT(f != nullptr);
    fputs("<alphabet name='Dup Test'><group name='Letters'>\n", f);
    fputs("<node label='x'><textCharAction /></node>\n", f);
    fputs("<node label='y'><textCharAction /></node>\n", f);
    fputs("<node label='z'><textCharAction /></node>\n", f);
    fputs("</group></alphabet>\n", f);
    fclose(f);

    dasher_ctx* ctx = dasher_create(dataDir, userDir, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Selecting the shared ID must load the MAINTAINED definition: 31
    // symbols, not the 3-letter legacy duplicate (4) and not a silent
    // builtin-Default fallback (27).
    dasher_set_alphabet_id(ctx, "Dup Test");
    const int symbols = dasher_get_alphabet_symbol_count(ctx);
    printf("  duplicate-ID resolution: %d symbols (want 31 = maintained variant)\n", symbols);
    ASSERT_EQ(symbols, 31);

    dasher_destroy(ctx);

    // Same corpus nested INSIDE a folder literally called "oldAlphabets":
    // the ancestor name must not collapse every file to the legacy tier
    // (the old substring-based tier check did, making the winner traversal
    // luck again).
    // Stale-state removal first (review): a reused PID with leftovers from
    // a previous run would resurrect old settings/files. ScopedTempDir
    // can't be used here — the data dir must live INSIDE a folder literally
    // named oldAlphabets — so clear the whole nested subtree manually.
    char nested[512], nestedData[512], nestedUser[512], nestedOld[512];
    snprintf(nested, sizeof(nested), "%s/oldAlphabets", dasher_temp_dir());
    snprintf(nestedData, sizeof(nestedData), "%s/nested_data_%d", nested, dasher_getpid());
    snprintf(nestedUser, sizeof(nestedUser), "%s/nested_user_%d", nested, dasher_getpid());
    snprintf(nestedOld, sizeof(nestedOld), "%s/oldAlphabets", nestedData);
    std::filesystem::remove_all(nestedData, ec);
    std::filesystem::remove_all(nestedUser, ec);
    std::filesystem::create_directories(nested, ec);
    std::filesystem::create_directories(nestedOld, ec);
    std::filesystem::create_directories(nestedUser, ec);

    f = fopen((std::string(nestedData) + "/alphabet.dup.xml").c_str(), "w");
    ASSERT(f != nullptr);
    fputs("<alphabet name='Dup Test'><group name='Letters'>\n", f);
    for (char c = 'a'; c <= 'z'; c++)
        fprintf(f, "<node label='%c'><textCharAction /></node>\n", c);
    fputs("<node label='&#x00e9;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00e8;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00ea;'><textCharAction /></node>\n", f);
    fputs("<node label='&#x00eb;'><textCharAction /></node>\n", f);
    fputs("</group></alphabet>\n", f);
    fclose(f);
    f = fopen((std::string(nestedOld) + "/alphabet.dup.legacy.xml").c_str(), "w");
    ASSERT(f != nullptr);
    fputs("<alphabet name='Dup Test'><group name='Letters'>\n", f);
    fputs("<node label='x'><textCharAction /></node>\n", f);
    fputs("<node label='y'><textCharAction /></node>\n", f);
    fputs("<node label='z'><textCharAction /></node>\n", f);
    fputs("</group></alphabet>\n", f);
    fclose(f);

    ctx = dasher_create(nestedData, nestedUser, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Dup Test");
    const int nestedSymbols = dasher_get_alphabet_symbol_count(ctx);
    printf("  nested-in-oldAlphabets ancestor: %d symbols (want 31 = maintained still wins)\n", nestedSymbols);
    ASSERT_EQ(nestedSymbols, 31);
    dasher_destroy(ctx);
}

TEST(alphabet_index_scanner_edge_cases) {
    // Regression (PR #68 review): the runtime name index used a raw string
    // scan that (a) missed single-quoted attributes, (b) stored XML
    // character references undecoded — 42 shipped files have entity-encoded
    // names like "Latvie&#353;u", which indexed under the encoded bytes and
    // then silently activated Default when selected — and (c) truncated at
    // a 2048-byte prefix. The indexer now uses pugixml itself. All three
    // edge cases live in one file: single quotes, numeric character
    // references, and a prolog pushing the root past 2048 bytes.
    // ScopedTempDir: stale-dir removal + RAII cleanup (a reused PID with a
    // leftover dasher_settings.xml made tests non-deterministic — the same
    // failure mode create_isolated_context() documents).
    ScopedTempDir dataDir, userDir;
    char path[512];
    snprintf(path, sizeof(path), "%s/alphabet.turkmen.entity.xml", dataDir.c_str());

    FILE* f = fopen(path, "w");
    ASSERT(f != nullptr);
    fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", f);
    // >2048 bytes of comment padding before the root element.
    for (int i = 0; i < 60; i++)
        fputs("<!-- padding padding padding padding padding padding padding padding -->\n", f);
    // Single-quoted attributes + numeric character references in the name:
    // decodes to "Türkänçe entity-test". (26 letters — the engine's node
    // expansion expects a non-degenerate alphabet.)
    fputs("<alphabet name='T&#x00FC;rk&#x00E4;n&#x00E7;e entity-test' orientation='LR'>\n", f);
    fputs("  <group name='Letters'>\n", f);
    for (char c = 'a'; c <= 'z'; c++)
        fprintf(f, "    <node label='%c'><textCharAction /></node>\n", c);
    fputs("  </group>\n", f);
    fputs("</alphabet>\n", f);
    fclose(f);

    dasher_ctx* ctx = dasher_create(dataDir, userDir, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // The menu must list the DECODED name, not the encoded bytes.
    // (Plain UTF-8 literal: the source file is UTF-8, so the string bytes
    // are the UTF-8 encoding of "Turkmance with diacritics".)
    const std::string expected = "T\xc3\xbc"
                                 "rk\xc3\xa4"
                                 "n\xc3\xa7"
                                 "e entity-test";
    bool listed = false;
    const int count = dasher_get_alphabet_count(ctx);
    for (int i = 0; i < count; i++) {
        if (expected == dasher_get_alphabet_name(ctx, i)) listed = true;
    }
    printf("  index edge-case alphabet listed with decoded name: %d (of %d)\n", listed, count);
    ASSERT(listed);

    // Selecting it must load it — not fall back to Default.
    dasher_set_alphabet_id(ctx, expected.c_str());
    const char* current = dasher_get_alphabet_id(ctx);
    printf("  selected '%s'\n", current ? current : "(null)");
    ASSERT(current != nullptr);
    ASSERT(std::string(current) == expected);

    dasher_destroy(ctx);
}

TEST(alphabet_without_training_file_stays_writable) {
    // Regression (#70): the trainer's "does not specify training file"
    // warning used FormatMessage — the MODAL path, which pauses the input
    // filter. The unpause path is only reachable after the model moves, so
    // a modal warning at engine start deadlocked Dasher permanently: every
    // alphabet without a (shipped) training corpus froze on first input —
    // 148 shipped alphabets. Informational warnings now use the
    // non-modal FormatInfoMessage; steering must keep working.
    // ScopedTempDir: stale-dir removal + RAII cleanup (same rationale as
    // the other tests in this file).
    ScopedTempDir dataDir, userDir;

    // Deliberately NO trainingFilename (and no corpus shipped for it).
    // Paragraph + space nodes included: the model's offset accounting
    // needs them (same shape as the shipped alphabets).
    FILE* f = fopen((std::string(dataDir) + "/alphabet.notrain.xml").c_str(), "w");
    ASSERT(f != nullptr);
    fputs("<alphabet name='NoTrain Test' orientation='LR'><group name='Letters'>\n", f);
    for (char c = 'a'; c <= 'z'; c++)
        fprintf(f, "<node label='%c'><textCharAction /></node>\n", c);
    fputs("</group>\n", f);
    fputs("<group name='paragraphSpace'>\n", f);
    fputs("<node label='&#182;' text='&#10;'><textCharAction /></node>\n", f);
    fputs("<node label='&#9633;'><textCharAction unicode='32' /></node>\n", f);
    fputs("</group></alphabet>\n", f);
    fclose(f);

    dasher_ctx* ctx = dasher_create(dataDir, userDir, nullptr);
    ASSERT(ctx != nullptr);
    // Select explicitly: without this the engine starts on the builtin
    // Default alphabet (the only alternative in a one-alphabet data dir),
    // which is a different test.
    dasher_set_alphabet_id(ctx, "NoTrain Test");
    dasher_set_screen_size(ctx, 800, 600);

    // The canonical steering pattern (as low_memory_text_output): if the
    // modal-pause deadlock returns, no frames move and output stays empty.
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 100; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000, 20);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    printf("  no-training output: '%s'\n", text ? text : "(null)");
    ASSERT(text != nullptr);
    ASSERT(strlen(text) > 0);

    dasher_destroy(ctx);
}

TEST(low_memory_frame_commands) {
    // Frame should still produce valid draw commands in low-memory mode
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_low_memory_mode(ctx, 1);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;

    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    ASSERT(commands != nullptr);
    ASSERT(cmd_count > 0);
    printf("  Frame produced %d command values\n", cmd_count);

    // Verify command count is a multiple of 6 (each command is 6 int32s)
    ASSERT_EQ(cmd_count % 6, 0);

    dasher_mouse_up(ctx);
    dasher_destroy(ctx);
}
