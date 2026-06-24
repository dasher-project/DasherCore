// Interaction loop tests: verify that mouse input produces text output
#include "test_common.h"
#include <chrono>
#include <cmath>
TEST(interaction_produces_text) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_speed_percent(ctx, 300);
    dasher_set_screen_size(ctx, 800, 600);

    // Move to center, start zooming
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    // Steer slightly up to target a letter, run many frames
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000 + i * 20);
    }

    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    printf("  Output after interaction: '%s' (len=%zu)\n", text, strlen(text));
    ASSERT(strlen(text) > 0);

    dasher_destroy(ctx);
}

TEST(interaction_callback_fires) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Track output via callback
    static char output_buf[1024] = {0};
    static int callback_count = 0;
    callback_count = 0;
    output_buf[0] = '\0';

    dasher_set_output_callback(
        ctx,
        [](int event_type, const char* text, void*) {
            if (event_type == 0 && text) {
                strncat(output_buf, text, sizeof(output_buf) - strlen(output_buf) - 1);
                callback_count++;
            }
        },
        nullptr);

    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000 + i * 20);
    }

    dasher_mouse_up(ctx);

    printf("  Callback fired %d times, output: '%s'\n", callback_count, output_buf);
    ASSERT(callback_count > 0);
    ASSERT(strlen(output_buf) > 0);

    dasher_destroy(ctx);
}

TEST(interaction_reset_clears) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_speed_percent(ctx, 300);
    dasher_set_screen_size(ctx, 800, 600);

    // Produce some text
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 300; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000 + i * 20);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(strlen(text) > 0);

    // Reset should clear output
    dasher_reset(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT_EQ(strlen(text), 0);

    dasher_destroy(ctx);
}

TEST(interaction_continuous_movement) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_speed_percent(ctx, 200);

    // Simulate sustained interaction: mouse down + continuous movement
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    size_t prev_len = 0;
    bool grew = false;

    for (int i = 0; i < 200; i++) {
        // Sweep mouse up and down
        float y = 300.0f + 100.0f * sin(i * 0.1f);
        dasher_mouse_move(ctx, 700.0f, y);
        run_frames(ctx, 1, 1000 + i * 20);

        const char* text = dasher_get_output_text(ctx);
        size_t cur_len = strlen(text);
        if (cur_len > prev_len) {
            grew = true;
            prev_len = cur_len;
        }
    }

    dasher_mouse_up(ctx);

    ASSERT(grew);
    const char* final_text = dasher_get_output_text(ctx);
    printf("  Final output: '%s' (len=%zu)\n", final_text, strlen(final_text));
    ASSERT(strlen(final_text) > 0);

    dasher_destroy(ctx);
}
