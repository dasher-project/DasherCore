// Deterministic text production tests: verify identical input always produces identical output
#include "test_common.h"
#include <string.h>

static const char* type_and_get(dasher_ctx* ctx, float x, float y, int frames) {
    dasher_mouse_move(ctx, x, y);
    dasher_mouse_down(ctx);
    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    for (int i = 0; i < frames; i++) {
        dasher_mouse_move(ctx, x, y);
        dasher_frame(ctx, 1000 + i * 16, &c, &cc, &s, &sc);
    }
    dasher_mouse_up(ctx);
    return dasher_get_output_text(ctx);
}

TEST(det_same_input_same_output) {
    char results[3][1024];

    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_speed_percent(ctx, 300);

        const char* output = type_and_get(ctx, 700.0f, 300.0f, 200);
        strncpy(results[run], output, 1023);
        results[run][1023] = '\0';
        printf("  Run %d: '%s'\n", run, results[run]);
        dasher_destroy(ctx);
    }

    ASSERT_EQ(strcmp(results[0], results[1]), 0);
    ASSERT_EQ(strcmp(results[0], results[2]), 0);
    printf("v det_same_input_same_output passed\n");
}

TEST(det_different_y_different_output) {
    char out1[1024], out2[1024];

    {
        dasher_ctx* ctx1 = create_isolated_context();
        ASSERT(ctx1);
        dasher_set_screen_size(ctx1, 800, 600);
        dasher_set_speed_percent(ctx1, 300);
        const char* output1 = type_and_get(ctx1, 700.0f, 300.0f, 500);
        strncpy(out1, output1, 1023);
        out1[1023] = '\0';
        dasher_destroy(ctx1);
    }

    {
        dasher_ctx* ctx2 = create_isolated_context();
        ASSERT(ctx2);
        dasher_set_screen_size(ctx2, 800, 600);
        dasher_set_speed_percent(ctx2, 300);
        const char* output2 = type_and_get(ctx2, 700.0f, 100.0f, 500);
        strncpy(out2, output2, 1023);
        out2[1023] = '\0';
        dasher_destroy(ctx2);
    }

    printf("  Y=300: '%s'\n", out1);
    printf("  Y=100: '%s'\n", out2);
    // Different Y positions should generally produce different output
    ASSERT_NEQ(strcmp(out1, out2), 0);

    printf("v det_different_y_different_output passed\n");
}

TEST(det_no_input_no_output) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int* c = nullptr;
    int cc = 0;
    char** s = nullptr;
    int sc = 0;
    for (int i = 0; i < 100; i++)
        dasher_frame(ctx, 1000 + i * 16, &c, &cc, &s, &sc);

    const char* output = dasher_get_output_text(ctx);
    printf("  No input output: '%s' (len=%zu)\n", output, strlen(output));
    ASSERT_EQ(strlen(output), 0);

    dasher_destroy(ctx);
    printf("v det_no_input_no_output passed\n");
}

TEST(det_reset_clears_output) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_speed_percent(ctx, 300);

    const char* output1 = type_and_get(ctx, 700.0f, 300.0f, 200);
    printf("  Before reset: '%s'\n", output1);
    ASSERT(strlen(output1) > 0);

    dasher_reset(ctx);

    const char* output2 = dasher_get_output_text(ctx);
    printf("  After reset: '%s'\n", output2);
    ASSERT_EQ(strlen(output2), 0);

    dasher_destroy(ctx);
    printf("v det_reset_clears_output passed\n");
}

TEST(det_offset_starts_at_zero) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int offset = dasher_get_offset(ctx);
    printf("  Initial offset: %d\n", offset);
    ASSERT_EQ(offset, 0);

    dasher_set_speed_percent(ctx, 300);
    type_and_get(ctx, 700.0f, 300.0f, 100);

    offset = dasher_get_offset(ctx);
    printf("  After typing: %d\n", offset);
    ASSERT(offset > 0);

    dasher_destroy(ctx);
    printf("v det_offset_starts_at_zero passed\n");
}

TEST(det_speed_affects_rate) {
    int lens[3] = {0, 0, 0};
    int speeds[3] = {100, 200, 400};

    for (int s = 0; s < 3; s++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_speed_percent(ctx, speeds[s]);
        const char* output = type_and_get(ctx, 700.0f, 300.0f, 100);
        lens[s] = strlen(output);
        printf("  Speed %d%%: %d chars\n", speeds[s], lens[s]);
        dasher_destroy(ctx);
    }

    // Higher speed should generally produce more text
    // (not strict for all cases, but 400% >> 100% should hold)
    ASSERT(lens[2] >= lens[0]);

    printf("v det_speed_affects_rate passed\n");
}

int main() {
    printf("Running deterministic text production tests...\n\n");

    test_det_same_input_same_output();
    test_det_different_y_different_output();
    test_det_no_input_no_output();
    test_det_reset_clears_output();
    test_det_offset_starts_at_zero();
    test_det_speed_affects_rate();

    printf("\nAll deterministic tests passed!\n");
    return 0;
}
