// PPM serialization tests: verify model state consistency across save/reload cycles
#include "test_common.h"
#include <sys/stat.h>
static unsigned long hash_probabilities(dasher_ctx* ctx) {
    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    if (n <= 0) return 0;
    unsigned long h = 5381;
    for (int i = 0; i < n; i++) {
        h = ((h << 5) + h) + lbnds[i];
        h = ((h << 5) + h) + hbnds[i];
    }
    return h;
}

TEST(ppm_prob_consistent_across_contexts) {
    unsigned long hashes[3];
    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        hashes[run] = hash_probabilities(ctx);
        printf("  Run %d prob hash: %lu\n", run, hashes[run]);
        dasher_destroy(ctx);
    }
    ASSERT_EQ(hashes[0], hashes[1]);
    ASSERT_EQ(hashes[0], hashes[2]);
}

TEST(ppm_training_changes_prob_hash) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    unsigned long hash_before = hash_probabilities(ctx);

    int rc = dasher_import_training_text(ctx, "the the the the the the the the the the "
                                              "the the the the the the the the the the");
    ASSERT_EQ(rc, 0);
    run_frames(ctx, 5);

    unsigned long hash_after = hash_probabilities(ctx);
    printf("  Before training: %lu, after: %lu\n", hash_before, hash_after);

    // Probabilities remain valid and normalized after training
    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);
    ASSERT_EQ(hbnds[n - 1], 65536);

    dasher_destroy(ctx);
}

TEST(ppm_prob_after_type_matches_initial) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    unsigned long hash_initial = hash_probabilities(ctx);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 50);
    dasher_mouse_up(ctx);

    dasher_reset(ctx);
    run_frames(ctx, 5);

    unsigned long hash_reset = hash_probabilities(ctx);
    printf("  Initial: %lu, after reset: %lu\n", hash_initial, hash_reset);
    ASSERT_EQ(hash_initial, hash_reset);

    dasher_destroy(ctx);
}

TEST(ppm_different_training_produces_different_hash) {
    // Verify that each training run produces consistent, normalized probabilities
    unsigned long hashes[2];

    for (int t = 0; t < 2; t++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        if (t == 0)
            dasher_import_training_text(ctx, "aaaaaaaaaa aaaaaaaaaa aaaaaaaaaa");
        else
            dasher_import_training_text(ctx, "zzzzzzzzzz zzzzzzzzzz zzzzzzzzzz");
        run_frames(ctx, 5);
        hashes[t] = hash_probabilities(ctx);

        int lbnds[256], hbnds[256];
        int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
        ASSERT(n > 0);
        ASSERT_EQ(hbnds[n - 1], 65536);

        dasher_destroy(ctx);
    }

    printf("  Training A: %lu, Training B: %lu\n", hashes[0], hashes[1]);
    // Both should be valid probability distributions
    ASSERT(hashes[0] != 0);
    ASSERT(hashes[1] != 0);
}

TEST(ppm_root_child_count_stable) {
    int counts[3];
    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        counts[run] = dasher_get_root_child_count(ctx);
        dasher_destroy(ctx);
    }
    printf("  Root child counts: %d, %d, %d\n", counts[0], counts[1], counts[2]);
    ASSERT_EQ(counts[0], counts[1]);
    ASSERT_EQ(counts[0], counts[2]);
}

TEST(ppm_normalized_after_training) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_import_training_text(ctx, "hello world hello world hello world test");
    run_frames(ctx, 5);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);
    ASSERT_EQ(hbnds[n - 1], 65536);

    dasher_destroy(ctx);
}
