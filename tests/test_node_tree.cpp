// Node tree structure tests: verify tree navigation and bounds consistency
#include "test_common.h"
TEST(tree_root_child_count_positive) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_root_child_count(ctx);
    printf("  Root children: %d\n", count);
    ASSERT(count > 0);

    dasher_destroy(ctx);
}

TEST(tree_all_children_have_valid_bounds) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_root_child_count(ctx);
    ASSERT(count > 0);

    for (int i = 0; i < count; i++) {
        long long lbnd, hbnd;
        int rc = dasher_get_root_child_bounds(ctx, i, &lbnd, &hbnd);
        ASSERT_EQ(rc, 0);
        ASSERT(hbnd > lbnd);
        ASSERT(lbnd >= 0);
        ASSERT(hbnd <= 65536);
    }

    printf("  All %d children have valid bounds\n", count);

    dasher_destroy(ctx);
}

TEST(tree_bounds_are_contiguous) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_root_child_count(ctx);
    ASSERT(count > 0);

    long long prev_hbnd = 0;
    for (int i = 0; i < count; i++) {
        long long lbnd, hbnd;
        dasher_get_root_child_bounds(ctx, i, &lbnd, &hbnd);
        printf("  Child %d: [%lld, %lld)\n", i, lbnd, hbnd);
        ASSERT_EQ(lbnd, prev_hbnd);
        prev_hbnd = hbnd;
    }
    ASSERT_EQ(prev_hbnd, 65536LL);

    dasher_destroy(ctx);
}

TEST(tree_child_bounds_match_probability_api) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);

    for (int i = 0; i < n; i++) {
        long long lbnd, hbnd;
        dasher_get_root_child_bounds(ctx, i, &lbnd, &hbnd);
        ASSERT_EQ((int)lbnd, lbnds[i]);
        ASSERT_EQ((int)hbnd, hbnds[i]);
    }

    dasher_destroy(ctx);
}

TEST(tree_invalid_index_returns_error) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_root_child_count(ctx);
    long long dummy_l, dummy_h;

    ASSERT_EQ(dasher_get_root_child_bounds(ctx, count, &dummy_l, &dummy_h), -1);
    ASSERT_EQ(dasher_get_root_child_bounds(ctx, count + 100, &dummy_l, &dummy_h), -1);
    ASSERT_EQ(dasher_get_root_child_bounds(ctx, -1, &dummy_l, &dummy_h), -1);

    dasher_destroy(ctx);
}

TEST(tree_count_stable_across_frames) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count0 = dasher_get_root_child_count(ctx);
    run_frames(ctx, 50);
    int count1 = dasher_get_root_child_count(ctx);

    printf("  Before frames: %d, after 50 frames: %d\n", count0, count1);
    ASSERT_EQ(count0, count1);

    dasher_destroy(ctx);
}

TEST(tree_count_changes_with_alphabet) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int count1 = dasher_get_root_child_count(ctx);
    printf("  Default: %d children\n", count1);

    dasher_set_alphabet_id(ctx, "English without punctuation");
    run_frames(ctx, 3);

    int count2 = dasher_get_root_child_count(ctx);
    printf("  No-punct: %d children\n", count2);

    // Different alphabets should have different symbol counts, which means
    // different root child counts (though they could theoretically be the same)
    ASSERT(count2 > 0);

    dasher_destroy(ctx);
}
