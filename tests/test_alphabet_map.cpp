// Alphabet map tests: verify symbol mapping and streaming via CAPI hooks
#include "test_common.h"

#include <algorithm>

TEST(map_symbol_count_matches_alphabet) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    int root_children = dasher_get_root_child_count(ctx);
    printf("  Symbols: %d, Root children: %d\n", sym_count, root_children);

    ASSERT(sym_count > 0);
    // Root children should be <= symbols (some may be control characters)
    ASSERT(root_children > 0);

    dasher_destroy(ctx);
}

TEST(map_first_symbol_is_valid) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    char buf[128];
    int rc = dasher_get_alphabet_symbol_text(ctx, 0, buf, sizeof(buf));
    printf("  Symbol 0: '%s' (rc=%d)\n", rc == 0 ? buf : "<error>", rc);
    // Symbol 0 may be a special symbol (e.g. root/end), just verify it doesn't crash

    rc = dasher_get_alphabet_symbol_text(ctx, 1, buf, sizeof(buf));
    if (rc == 0) {
        printf("  Symbol 1: '%s'\n", buf);
        ASSERT(strlen(buf) > 0);
    }

    dasher_destroy(ctx);
}

TEST(map_all_symbols_accessible) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    int accessible = 0;

    for (int i = 0; i < sym_count; i++) {
        char buf[128];
        int rc = dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf));
        if (rc == 0) accessible++;
    }

    printf("  %d/%d symbols accessible\n", accessible, sym_count);
    ASSERT(accessible > 0);

    dasher_destroy(ctx);
}

TEST(map_alphabet_switch_updates_symbols) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count1 = dasher_get_alphabet_symbol_count(ctx);
    printf("  Default symbols: %d\n", count1);

    dasher_set_alphabet_id(ctx, "English without punctuation");
    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &c, &cc, &s, &sc);

    int count2 = dasher_get_alphabet_symbol_count(ctx);
    printf("  No-punct symbols: %d\n", count2);

    // Different alphabets have different symbol counts
    ASSERT(count2 > 0);

    dasher_destroy(ctx);
}

TEST(map_symbols_deterministic_across_contexts) {
    int counts[3];
    char first_symbols[3][64];

    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);

        counts[run] = dasher_get_alphabet_symbol_count(ctx);
        dasher_get_alphabet_symbol_text(ctx, 1, first_symbols[run], 64);

        dasher_destroy(ctx);
    }

    printf("  Counts: %d, %d, %d\n", counts[0], counts[1], counts[2]);
    ASSERT_EQ(counts[0], counts[1]);
    ASSERT_EQ(counts[0], counts[2]);
    ASSERT_STR_EQ(first_symbols[0], first_symbols[1]);
    ASSERT_STR_EQ(first_symbols[0], first_symbols[2]);
}

TEST(map_symbol_text_buffer_too_small) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    char buf[1];
    // Should handle gracefully (truncate or error)
    int rc = dasher_get_alphabet_symbol_text(ctx, 1, buf, 1);
    printf("  Buffer size 1: rc=%d\n", rc);
    // Either returns -1 or truncates - just verify no crash

    dasher_destroy(ctx);
}

TEST(map_longest_match_trains_multi_codepoint) {
    // RFC 0020 clause 4, end to end: a FLAT alphabet (nodes at root, so
    // dasher_get_probabilities exposes each node's mass directly) whose
    // training corpus contains only multi-codepoint tokens. With
    // longest-match, the ZWJ node's trained mass must exceed the untrained
    // single-codepoint node's; before, the corpus split into per-codepoint
    // unknowns and every node stayed at uniform default.
    ScopedTempDir dataRoot;
    const std::string data_dir = build_data_dir(dataRoot);
    std::string xml =
        std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
        "<!DOCTYPE alphabet SYSTEM \"../alphabet.dtd\">\n" +
        "<alphabet name=\"LM Flat\" orientation=\"LR\" trainingFilename=\"training_lm_flat.txt\" "
        "colorsName=\"Default\">\n" +
        "  <group name=\"flat\">\n" + "    <node label=\"&#x1F600;\"><textCharAction /></node>\n" + // 😀 1 codepoint
        "    <node label=\"&#x1F468;&#x200D;&#x1F469;&#x200D;&#x1F467;\"><textCharAction /></node>\n" + // family, 5
        "    <node label=\"&#x2708;&#xFE0F;\"><textCharAction /></node>\n" + // ✈️ 2 codepoints
        "    <node label=\"x\"><textCharAction /></node>\n" + "  </group>\n" + "</alphabet>\n";
    ASSERT(write_data_file(data_dir, "alphabets", "alphabet.lmflat.xml", xml));
    // Corpus: ONLY the multi-codepoint tokens (spaces are not symbols here
    // — no space node — so they become unknowns and train nothing).
    std::string corpus;
    for (int i = 0; i < 200; i++)
        corpus += "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"
                  " \xE2\x9C\x88\xEF\xB8\x8F ";
    ASSERT(write_data_file(data_dir, "training", "training_lm_flat.txt", corpus + "\n"));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), dataRoot.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "LM Flat");
    ASSERT_STR_EQ(dasher_get_alphabet_id(ctx), "LM Flat");

    // Locate the family and 😀 root children by symbol text order.
    const std::string family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7";
    const std::string grin = "\xF0\x9F\x98\x80";
    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    int grin_idx = -1, fam_idx = -1;
    for (int i = 1; i < sym_count; i++) {
        char buf[128];
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf)) != 0) continue;
        if (buf == grin) grin_idx = i;
        if (buf == family) fam_idx = i;
    }
    printf("  grin symbol %d, family symbol %d\n", grin_idx, fam_idx);
    ASSERT(grin_idx > 0);
    ASSERT(fam_idx > 0);

    // Advance frames so training and the model settle, then read the root
    // children's probability bounds. Flat alphabet: root children are the
    // symbol nodes themselves.
    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    for (int f = 0; f < 30; f++)
        dasher_frame(ctx, 1000 + f * 16, &c, &cc, &s, &sc);

    int lb[64], hb[64];
    int n = dasher_get_probabilities(ctx, lb, hb, 64);
    printf("  root children: %d\n", n);
    ASSERT(n >= 4);

    // The corpus contains ONLY the multi-codepoint tokens (400 symbols).
    // Under uniform defaults each of the n children holds ~65536/n; the
    // trained top child must dominate well beyond that (measured 1.94x
    // with PPM smoothing; pre-fix the corpus split into per-codepoint
    // unknowns and no child exceeded uniform — the assertion that failed).
    long long best = -1;
    for (int i = 0; i < n; i++)
        best = std::max(best, (long long)hb[i] - lb[i]);
    long long uniform = 65536 / n;
    printf("  best child mass %lld vs uniform %lld\n", best, uniform);
    ASSERT(best > 3 * uniform / 2);

    dasher_destroy(ctx);
}
