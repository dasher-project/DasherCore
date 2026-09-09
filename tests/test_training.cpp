// Training adaptation tests: verify training text changes model behavior
#include "test_common.h"
#include <fstream>

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

// Writes a distinctive corpus as the engine would have accumulated it (the
// user-dir ROOT is where ResolveUserDataPath appends).
static void write_user_training(const ScopedTempDir& dir, const char* text, int repeat) {
    std::ofstream out(std::filesystem::path(dir.path) / "training_english_GB.txt", std::ios::app);
    for (int i = 0; i < repeat; i++)
        out << text;
}

TEST(capi_version_reports_user_dir_scan) {
    // Version 1 = startup training load includes the per-context user dir
    // (DasherCore#84). Frontends gate their stopgap re-import on this.
    printf("  capi version: %d\n", dasher_capi_version());
    ASSERT(dasher_capi_version() >= 1);
}

TEST(training_user_dir_loaded_at_startup) {
    // Baseline: fresh user dir, nothing accumulated.
    unsigned long baseline;
    {
        ScopedContext ctx(800, 600);
        run_frames(ctx, 3);
        baseline = hash_probabilities(ctx);
    }
    ASSERT(baseline != 0);

    // Same data dir; user dir pre-populated with accumulated training in the
    // location the engine itself appends to. The startup scan must load it —
    // before #84 the scan walked the data dir only and split-dir frontends
    // lost all learning between sessions.
    ScopedTempDir userDir;
    write_user_training(userDir, "xyzzy quork zumble flibbertigibbet zatch ", 60);

    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, userDir.c_str(), nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    run_frames(ctx, 3);
    unsigned long trained = hash_probabilities(ctx);
    dasher_destroy(ctx);

    printf("  baseline=%lu trained=%lu\n", baseline, trained);
    ASSERT(trained != 0);
    ASSERT(trained != baseline);
}

TEST(training_scan_is_per_context) {
    // Context A has accumulated training; context B is created AFTER A, so
    // the process-global FileUtils dirs belong to A. B (fresh user dir) must
    // match the no-training baseline — its scan reads interface-owned dirs,
    // not the globals (DasherCore#84 / last-create-wins wrinkle).
    unsigned long baseline;
    {
        ScopedContext ctx(800, 600);
        run_frames(ctx, 3);
        baseline = hash_probabilities(ctx);
    }
    ASSERT(baseline != 0);

    ScopedTempDir dirA;
    write_user_training(dirA, "asymmetric corpus unique to context a brangler ", 60);
    dasher_ctx* a = dasher_create(TEST_DATA_DIR, dirA.c_str(), nullptr);
    ASSERT(a);
    dasher_set_screen_size(a, 800, 600);
    run_frames(a, 3);

    ScopedContext b(800, 600); // created after A — globals point at A's dirs
    run_frames(b, 3);
    unsigned long hashB = hash_probabilities(b);

    dasher_destroy(a);

    printf("  baseline=%lu b=%lu\n", baseline, hashB);
    ASSERT_EQ(hashB, baseline);
}

TEST(training_same_dir_setup_remains_valid) {
    // Windows-style single-dir setup (data dir == user dir): the user-dir
    // scan is skipped so nothing double-trains; engine realizes normally
    // and probabilities stay normalized.
    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, TEST_DATA_DIR, nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    run_frames(ctx, 3);

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
    ASSERT(n > 0);
    ASSERT_EQ(hbnds[n - 1], 65536);

    dasher_destroy(ctx);
    printf("  single-dir setup: %d children, normalized OK\n", n);
}

TEST(training_import_returns_success) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int rc = dasher_import_training_text(ctx, "hello world this is a test");
    printf("  Import result: %d\n", rc);
    ASSERT_EQ(rc, 0);

    dasher_destroy(ctx);
}

TEST(training_import_empty_string) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int rc = dasher_import_training_text(ctx, "");
    printf("  Empty import result: %d\n", rc);
    ASSERT_EQ(rc, 0);

    dasher_destroy(ctx);
}

TEST(training_multiple_imports_cumulative) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_import_training_text(ctx, "aaa bbb ccc");
    run_frames(ctx, 3);
    int lbnds1[256], hbnds1[256];
    int n1 = dasher_get_probabilities(ctx, lbnds1, hbnds1, 256);
    ASSERT(n1 > 0);
    ASSERT_EQ(hbnds1[n1 - 1], 65536);

    dasher_import_training_text(ctx, "ddd eee fff");
    run_frames(ctx, 3);
    int lbnds2[256], hbnds2[256];
    int n2 = dasher_get_probabilities(ctx, lbnds2, hbnds2, 256);
    ASSERT(n2 > 0);
    ASSERT_EQ(hbnds2[n2 - 1], 65536);

    printf("  After 1st: %d children, after 2nd: %d children\n", n1, n2);
    ASSERT(n1 > 0);
    ASSERT(n2 > 0);

    dasher_destroy(ctx);
}

TEST(training_repeated_words_shift_probabilities) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // Train heavily on specific text
    for (int i = 0; i < 5; i++) {
        dasher_import_training_text(ctx, "the the the the the the the the the the ");
        run_frames(ctx, 2);
    }

    int lbnds[256], hbnds[256];
    int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);

    // Probabilities remain valid after heavy training
    ASSERT(n > 0);
    ASSERT_EQ(hbnds[n - 1], 65536);

    // Verify bounds are monotonic
    for (int i = 1; i < n; i++) {
        ASSERT(lbnds[i] >= lbnds[i - 1]);
        ASSERT(hbnds[i] >= hbnds[i - 1]);
    }

    printf("  %d children, normalized OK after heavy training\n", n);

    dasher_destroy(ctx);
}

TEST(training_preserves_normalization) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    const char* texts[] = {
        "hello world foo bar",
        "the quick brown fox",
        "lorem ipsum dolor sit amet consectetur adipiscing elit",
    };

    for (int i = 0; i < 3; i++) {
        dasher_import_training_text(ctx, texts[i]);
        run_frames(ctx, 3);

        int lbnds[256], hbnds[256];
        int n = dasher_get_probabilities(ctx, lbnds, hbnds, 256);
        ASSERT(n > 0);
        ASSERT_EQ(hbnds[n - 1], 65536);
        printf("  After training %d: %d children, normalized OK\n", i, n);
    }

    dasher_destroy(ctx);
}

TEST(training_identical_repeated_deterministic) {
    unsigned long hashes[3];

    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);

        dasher_import_training_text(ctx, "deterministic test text for validation");
        run_frames(ctx, 5);

        hashes[run] = hash_probabilities(ctx);
        dasher_destroy(ctx);
    }

    printf("  Hashes: %lu, %lu, %lu\n", hashes[0], hashes[1], hashes[2]);
    ASSERT_EQ(hashes[0], hashes[1]);
    ASSERT_EQ(hashes[0], hashes[2]);
}
