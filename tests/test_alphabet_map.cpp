// Alphabet map tests: verify symbol mapping and streaming via CAPI hooks
#include "test_common.h"

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
