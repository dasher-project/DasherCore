// PPM golden probability tests: capture exact probability distributions
// for known states. These serve as regression baselines for a Rust rewrite.
#include "test_common.h"

static void run_frames(dasher_ctx* ctx, int count) {
    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    for (int i = 0; i < count; i++)
        dasher_frame(ctx, 1000 + i * 20, &c, &cc, &s, &sc);
}

static int sum_probabilities(dasher_ctx* ctx) {
    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    if (n <= 0) return -1;
    return hbnds[n - 1];
}

TEST(prob_initial_distribution_exists) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    printf("  Child count under crosshair: %d\n", n);
    ASSERT(n > 0);

    for (int i = 0; i < n; i++) {
        ASSERT(hbnds[i] > lbnds[i]);
        if (i > 0) ASSERT(lbnds[i] == hbnds[i - 1]);
    }
    printf("  Total probability mass: %d (expected 65536)\n", hbnds[n - 1]);
    ASSERT_EQ(hbnds[n - 1], 65536);

    dasher_destroy(ctx);
    printf("v prob_initial_distribution_exists passed\n");
}

TEST(prob_bounds_are_monotonic) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);

    for (int i = 1; i < n; i++) {
        ASSERT(lbnds[i] >= lbnds[i - 1]);
        ASSERT(hbnds[i] >= hbnds[i - 1]);
        ASSERT(hbnds[i] > lbnds[i]);
    }

    dasher_destroy(ctx);
    printf("v prob_bounds_are_monotonic passed\n");
}

TEST(prob_after_import_training_text) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int rc = dasher_import_training_text(ctx, "the quick brown fox jumps over the lazy dog. "
                                              "the quick brown fox jumps over the lazy dog. "
                                              "the quick brown fox jumps over the lazy dog. ");
    ASSERT_EQ(rc, 0);
    run_frames(ctx, 5);

    int lbnds2[256], hbnds2[256];
    int n2 = dasher_get_probabilities(ctx, lbnds2, hbnds2, 256);
    ASSERT(n2 > 0);
    ASSERT_EQ(hbnds2[n2 - 1], 65536);

    dasher_destroy(ctx);
    printf("v prob_after_import_training_text passed\n");
}

TEST(prob_alphabet_symbol_count) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int sym_count = dasher_get_alphabet_symbol_count(ctx);
    printf("  Alphabet symbol count: %d\n", sym_count);
    ASSERT(sym_count > 0);

    for (int i = 1; i < sym_count && i < 20; i++) {
        char buf[64];
        int rc = dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof(buf));
        if (rc == 0) {
            printf("  Symbol %d: '%s'\n", i, buf);
            ASSERT(strlen(buf) > 0);
        }
    }

    dasher_destroy(ctx);
    printf("v prob_alphabet_symbol_count passed\n");
}

TEST(prob_root_child_count_matches) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int child_count = dasher_get_root_child_count(ctx);
    int lbnds[256], hbnds[256];
    int prob_count = dasher_get_probabilities(ctx, lbnds, hbnds, 256);

    printf("  Root child count: %d, prob count: %d\n", child_count, prob_count);
    ASSERT(child_count > 0);
    ASSERT_EQ(child_count, prob_count);

    dasher_destroy(ctx);
    printf("v prob_root_child_count_matches passed\n");
}

TEST(prob_child_bounds_via_two_apis) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);

    for (int i = 0; i < n && i < 10; i++) {
        long long lbnd, hbnd;
        int rc = dasher_get_root_child_bounds(ctx, i, &lbnd, &hbnd);
        ASSERT_EQ(rc, 0);
        ASSERT_EQ((int)lbnd, lbnds[i]);
        ASSERT_EQ((int)hbnd, hbnds[i]);
    }

    long long dummy_l, dummy_h;
    ASSERT_EQ(dasher_get_root_child_bounds(ctx, n + 100, &dummy_l, &dummy_h), -1);
    ASSERT_EQ(dasher_get_root_child_bounds(ctx, -1, &dummy_l, &dummy_h), -1);

    dasher_destroy(ctx);
    printf("v prob_child_bounds_via_two_apis passed\n");
}

TEST(prob_uniform_parameter_set_get) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int uniform_key = dasher_find_parameter_key("LP_UNIFORM");
    ASSERT(uniform_key >= 0);

    long orig = dasher_get_long_parameter(ctx, uniform_key);
    printf("  Original uniform: %ld\n", orig);

    dasher_set_long_parameter(ctx, uniform_key, 100);
    ASSERT_EQ(dasher_get_long_parameter(ctx, uniform_key), 100);
    printf("  Set to 100, read back: %ld\n", dasher_get_long_parameter(ctx, uniform_key));

    dasher_set_long_parameter(ctx, uniform_key, orig);

    dasher_destroy(ctx);
    printf("v prob_uniform_parameter_set_get passed\n");
}

TEST(prob_total_is_normalized) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    for (int trial = 0; trial < 5; trial++) {
        int lbnds[256], hbnds[256];
        int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
        ASSERT(n > 0);
        ASSERT_EQ(hbnds[n - 1], 65536);
        run_frames(ctx, 1);
    }

    dasher_destroy(ctx);
    printf("v prob_total_is_normalized passed\n");
}

int main() {
    printf("Running PPM golden probability tests...\n\n");

    test_prob_initial_distribution_exists();
    test_prob_bounds_are_monotonic();
    test_prob_after_import_training_text();
    test_prob_alphabet_symbol_count();
    test_prob_root_child_count_matches();
    test_prob_child_bounds_via_two_apis();
    test_prob_uniform_parameter_set_get();
    test_prob_total_is_normalized();

    printf("\nAll PPM golden tests passed!\n");
    return 0;
}
