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
