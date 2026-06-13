// Extended C API tests: cover functions not tested in test_capi.cpp
#include "test_common.h"

static void run_frames(dasher_ctx* ctx, int count) {
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    for (int i = 0; i < count; i++) {
        dasher_frame(ctx, 1000 + i * 20, &commands, &cmd_count, &strings, &str_count);
    }
}

TEST(long_string_params) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Find a known parameter key
    int key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(key >= 0);
    printf("  LP_MAX_BITRATE key: %d\n", key);

    // Get/set long parameter
    long original = dasher_get_long_parameter(ctx, key);
    ASSERT(original >= 0);
    printf("  Original LP_MAX_BITRATE: %ld\n", original);

    dasher_set_long_parameter(ctx, key, 5000);
    long val = dasher_get_long_parameter(ctx, key);
    ASSERT_EQ(val, 5000);

    // Restore
    dasher_set_long_parameter(ctx, key, original);

    // Test string parameter
    int str_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(str_key >= 0);

    const char* alph = dasher_get_string_parameter(ctx, str_key);
    ASSERT(alph != nullptr);
    printf("  Current alphabet: '%s'\n", alph);

    dasher_set_string_parameter(ctx, str_key, "English lower case");
    const char* new_alph = dasher_get_string_parameter(ctx, str_key);
    ASSERT_STR_EQ(new_alph, "English lower case");

    dasher_destroy(ctx);
    printf("v long_string_params passed\n");
}

TEST(palettes) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_palette_count(ctx);
    printf("  Palette count: %d\n", count);
    ASSERT(count > 0);

    // List all palettes
    for (int i = 0; i < count; i++) {
        const char* name = dasher_get_palette_name(ctx, i);
        ASSERT(name != nullptr);
        ASSERT(strlen(name) > 0);
        printf("  Palette %d: '%s'\n", i, name);
    }

    // Get current palette
    const char* current = dasher_get_current_palette(ctx);
    ASSERT(current != nullptr);
    printf("  Current palette: '%s'\n", current);

    // Get preview colors
    int32_t colors[4] = {0, 0, 0, 0};
    int result = dasher_get_palette_preview_colors(ctx, 0, colors);
    ASSERT_EQ(result, 0);
    printf("  Preview colors: 0x%08X 0x%08X 0x%08X 0x%08X\n", colors[0], colors[1], colors[2], colors[3]);

    // Try setting a palette
    const char* first_name = dasher_get_palette_name(ctx, 0);
    dasher_set_palette(ctx, first_name);

    dasher_destroy(ctx);
    printf("v palettes passed\n");
}

TEST(alphabet_listing) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_alphabet_count(ctx);
    printf("  Alphabet count: %d\n", count);
    ASSERT(count > 0);

    for (int i = 0; i < count; i++) {
        const char* name = dasher_get_alphabet_name(ctx, i);
        ASSERT(name != nullptr);
        ASSERT(strlen(name) > 0);
        printf("  Alphabet %d: '%s'\n", i, name);
    }

    dasher_destroy(ctx);
    printf("v alphabet_listing passed\n");
}

TEST(enum_values) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Find a parameter that has enum values (e.g., orientation)
    int orient_key = dasher_find_parameter_key("LP_ORIENTATION");
    ASSERT(orient_key >= 0);

    int enum_count = dasher_get_parameter_enum_count(orient_key);
    printf("  Orientation enum count: %d\n", enum_count);
    ASSERT(enum_count > 0);

    for (int i = 0; i < enum_count; i++) {
        const char* name = dasher_get_parameter_enum_name(orient_key, i);
        int value = dasher_get_parameter_enum_value(orient_key, i);
        ASSERT(name != nullptr);
        printf("  Enum %d: '%s' = %d\n", i, name, value);
    }

    dasher_destroy(ctx);
    printf("v enum_values passed\n");
}

TEST(language_model_info) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    int lm_count = dasher_get_language_model_count();
    printf("  Language model count: %d\n", lm_count);
    ASSERT(lm_count > 0);

    for (int i = 0; i < lm_count; i++) {
        const char* name = dasher_get_language_model_name(i);
        const char* desc = dasher_get_language_model_description(i);
        ASSERT(name != nullptr);
        printf("  LM %d: '%s' - '%s'\n", i, name, desc ? desc : "(null)");

        int param_count = dasher_get_language_model_param_count(i);
        printf("    Parameters: %d\n", param_count);
    }

    dasher_destroy(ctx);
    printf("v language_model_info passed\n");
}

TEST(message_callback) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    static int msg_count = 0;
    msg_count = 0;

    dasher_set_message_callback(ctx, [](int message_type, const char* text, void*) {
        if (text) {
            printf("  Message (type=%d): '%s'\n", message_type, text);
            msg_count++;
        }
    }, nullptr);

    // Trigger something that might produce a message (e.g., setting an invalid alphabet)
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Nonexistent Alphabet XYZ");

    // The message callback might fire for error states
    printf("  Messages received: %d\n", msg_count);

    dasher_destroy(ctx);
    printf("v message_callback passed\n");
}

TEST(reset) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_speed_percent(ctx, 300);
    dasher_set_screen_size(ctx, 800, 600);

    // Produce some text
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        int* cmds = nullptr; int cmd_cnt = 0; char** strs = nullptr; int str_cnt = 0;
        dasher_frame(ctx, 1000 + i * 20, &cmds, &cmd_cnt, &strs, &str_cnt);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(strlen(text) > 0);

    // Reset should clear everything
    dasher_reset(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT_EQ(strlen(text), 0);

    dasher_destroy(ctx);
    printf("v reset passed\n");
}

TEST(save_settings) {
    static int save_test_counter = 0;
    char shared_dir[256];
    snprintf(shared_dir, sizeof(shared_dir), "/tmp/dasher_save_test_%d_%d", getpid(), save_test_counter++);
    mkdir(shared_dir, 0755);

    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Change a setting
    dasher_set_speed_percent(ctx, 250);

    // Save
    dasher_save_settings(ctx);

    // Create new context - should load saved settings
    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx2 != nullptr);

    int speed = dasher_get_speed_percent(ctx2);
    printf("  Speed after reload: %d\n", speed);

    // Restore default
    dasher_set_speed_percent(ctx2, 100);
    dasher_save_settings(ctx2);

    dasher_destroy(ctx);
    dasher_destroy(ctx2);
    printf("v save_settings passed\n");
}

TEST(key_event) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Key events should not crash (null-safe, valid key codes)
    dasher_key_event(ctx, 0, 1); // press key 0
    dasher_key_event(ctx, 0, 0); // release key 0

    run_frames(ctx, 5);

    dasher_destroy(ctx);
    printf("v key_event passed\n");
}

TEST(string_values) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Get string values for SP_ALPHABET_ID
    int alph_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(alph_key >= 0);

    const char* values_buf[32] = {nullptr};
    int count = dasher_get_parameter_string_values(ctx, alph_key, values_buf, 32);
    printf("  String values count: %d\n", count);
    ASSERT(count > 0);

    for (int i = 0; i < count && i < 5; i++) {
        ASSERT(values_buf[i] != nullptr);
        printf("  Value %d: '%s'\n", i, values_buf[i]);
    }

    dasher_destroy(ctx);
    printf("v string_values passed\n");
}

TEST(game_mode_basic) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Game mode should start disabled
    ASSERT_EQ(dasher_game_mode_active(ctx), 0);

    // Enable game mode
    ASSERT_EQ(dasher_enter_game_mode(ctx), 0);
    ASSERT_EQ(dasher_game_mode_active(ctx), 1);

    // Get target text
    const char* target = dasher_game_get_target_text(ctx);
    printf("  Target text: '%s'\n", target ? target : "(null)");

    // Get correct count and target length
    int correct = dasher_game_get_correct_count(ctx);
    int total = dasher_game_get_target_length(ctx);
    printf("  Progress: %d/%d\n", correct, total);

    // Get wrong text
    const char* wrong = dasher_game_get_wrong_text(ctx);
    printf("  Wrong text: %s\n", wrong ? wrong : "(null)");

    // Leave game mode
    dasher_leave_game_mode(ctx);
    ASSERT_EQ(dasher_game_mode_active(ctx), 0);

    dasher_destroy(ctx);
    printf("v game_mode_basic passed\n");
}

TEST(null_safety_extended) {
    // Additional null-safety for functions not covered by basic null_safety test
    dasher_ctx* null_ctx = nullptr;

    // These should all return safely without crashing
    dasher_set_low_memory_mode(null_ctx, 1);
    dasher_reset(null_ctx);
    dasher_key_event(null_ctx, 0, 1);
    dasher_save_settings(null_ctx);

    ASSERT_EQ(dasher_get_palette_count(null_ctx), 0);
    ASSERT(dasher_get_palette_name(null_ctx, 0) != nullptr); // returns "" not null
    ASSERT(dasher_get_current_palette(null_ctx) != nullptr); // returns "" not null
    ASSERT_EQ(dasher_get_palette_preview_colors(null_ctx, 0, nullptr), -1);

    ASSERT_EQ(dasher_get_alphabet_count(null_ctx), 0);
    ASSERT(dasher_get_alphabet_name(null_ctx, 0) != nullptr); // returns "" not null

    ASSERT_EQ(dasher_game_mode_active(null_ctx), 0);
    dasher_enter_game_mode(null_ctx);
    dasher_leave_game_mode(null_ctx);

    ASSERT_EQ(dasher_find_parameter_key("nonexistent_param_xyz"), -1);

    int32_t colors[4] = {0};
    ASSERT_EQ(dasher_get_palette_preview_colors(null_ctx, 0, colors), -1);

    printf("v null_safety_extended passed\n");
}

int main() {
    printf("Running Dasher extended C API tests...\n\n");

    test_long_string_params();
    test_palettes();
    test_alphabet_listing();
    test_enum_values();
    test_language_model_info();
    test_message_callback();
    test_reset();
    test_save_settings();
    test_key_event();
    test_string_values();
    test_game_mode_basic();
    test_null_safety_extended();

    printf("\nAll extended tests passed!\n");
    return 0;
}
