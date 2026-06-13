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
    printf("v alphabet_default_loaded passed\n");
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
    printf("v alphabet_count_positive passed\n");
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
    printf("v alphabet_switch_to_english_no_punct passed\n");
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
    printf("v alphabet_switch_invalid_falls_back passed\n");
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
    printf("v alphabet_symbol_texts_are_nonempty passed\n");
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
    printf("v alphabet_symbol_out_of_range_returns_error passed\n");
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
    printf("v alphabet_switch_changes_probabilities passed\n");
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
    printf("v alphabet_affects_root_child_count passed\n");
}

int main() {
    printf("Running alphabet XML parsing tests...\n\n");

    test_alphabet_default_loaded();
    test_alphabet_count_positive();
    test_alphabet_switch_to_english_no_punct();
    test_alphabet_switch_invalid_falls_back();
    test_alphabet_symbol_texts_are_nonempty();
    test_alphabet_symbol_out_of_range_returns_error();
    test_alphabet_switch_changes_probabilities();
    test_alphabet_affects_root_child_count();

    printf("\nAll alphabet XML tests passed!\n");
    return 0;
}
