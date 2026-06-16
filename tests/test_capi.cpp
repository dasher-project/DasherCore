// Basic tests for the Dasher C API
// These tests provide a contract that frontends can verify

#include "test_common.h"

TEST(color_utilities) {
    // Test basic color creation
    int white = dasher_color_rgb(255, 255, 255);
    ASSERT_EQ(white, (int)0xFFFFFFFF);

    int black = dasher_color_rgb(0, 0, 0);
    ASSERT_EQ(black, (int)0xFF000000);

    // Test with alpha
    int semi_red = dasher_color_argb(128, 255, 0, 0);
    ASSERT_EQ(semi_red, (int)0x80FF0000);

    // Test component extraction
    ASSERT_EQ(dasher_color_get_alpha(0x12345678), 0x12);
    ASSERT_EQ(dasher_color_get_red(0x12345678), 0x34);
    ASSERT_EQ(dasher_color_get_green(0x12345678), 0x56);
    ASSERT_EQ(dasher_color_get_blue(0x12345678), 0x78);

    printf("✓ color_utilities passed\n");
}

TEST(context_creation) {
    const char* data_dir = get_test_data_dir();
    printf("  Using data directory: %s\n", data_dir);

    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Test that context is functional
    const char* alphabet = dasher_get_alphabet_id(ctx);
    ASSERT(alphabet != nullptr);
    printf("  Got alphabet: '%s' (length: %zu)\n", alphabet, strlen(alphabet));

    // Some implementations might return empty string if not fully initialized
    if (strlen(alphabet) > 0) {
        printf("✓ context_creation passed\n");
    } else {
        printf("⚠ context_creation: alphabet empty (may be expected if data dir not found)\n");
    }

    dasher_destroy(ctx);
}

TEST(screen_size) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Set screen size
    dasher_set_screen_size(ctx, 800, 600);

    // Get frame data (should work after screen size is set)
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;

    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    // Should have some commands (at minimum clear screen)
    ASSERT(commands != nullptr);
    ASSERT(cmd_count > 0);

    dasher_destroy(ctx);
    printf("✓ screen_size passed\n");
}

TEST(parameters) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Test speed parameter
    int speed = dasher_get_speed_percent(ctx);
    ASSERT(speed >= 20 && speed <= 400);

    dasher_set_speed_percent(ctx, 150);
    speed = dasher_get_speed_percent(ctx);
    ASSERT(speed == 150);

    // Test language model parameter
    int model = dasher_get_language_model_id(ctx);
    ASSERT(model >= 0);

    dasher_set_language_model_id(ctx, 2);
    model = dasher_get_language_model_id(ctx);
    ASSERT_EQ(model, 2);

    // Test boolean parameter
    int start_on_space = dasher_get_bool_parameter(ctx, 0); // BP_START_ON_SPACE
    ASSERT(start_on_space == 0 || start_on_space == 1);

    dasher_set_bool_parameter(ctx, 0, 1);
    start_on_space = dasher_get_bool_parameter(ctx, 0);
    ASSERT_EQ(start_on_space, 1);

    dasher_destroy(ctx);
    printf("✓ parameters passed\n");
}

TEST(output_text) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);

    // Initially empty
    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    ASSERT_EQ(strlen(text), 0);

    // Reset should keep it empty
    dasher_reset_output_text(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT_EQ(strlen(text), 0);

    dasher_destroy(ctx);
    printf("✓ output_text passed\n");
}

TEST(alphabet) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Get default alphabet
    const char* alphabet = dasher_get_alphabet_id(ctx);
    ASSERT(alphabet != nullptr);
    ASSERT(strlen(alphabet) > 0);

    // Try to set a different alphabet (this may fail if alphabet doesn't exist)
    const char* original_alphabet = strdup(alphabet);
    dasher_set_alphabet_id(ctx, "English");

    // Reset to original
    dasher_set_alphabet_id(ctx, original_alphabet);
    const char* new_alphabet = dasher_get_alphabet_id(ctx);
    ASSERT_STR_EQ(new_alphabet, original_alphabet);

    free((void*)original_alphabet);
    dasher_destroy(ctx);
    printf("✓ alphabet passed\n");
}

TEST(null_safety) {
    // Test that null contexts are handled safely
    dasher_ctx* null_ctx = nullptr;

    // All these should handle null gracefully without crashing
    dasher_destroy(null_ctx);
    dasher_set_screen_size(null_ctx, 800, 600);
    dasher_mouse_move(null_ctx, 100.0f, 100.0f);
    dasher_mouse_down(null_ctx);
    dasher_mouse_up(null_ctx);

    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(null_ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    ASSERT(dasher_get_output_text(null_ctx) != nullptr);
    dasher_reset_output_text(null_ctx);
    ASSERT(dasher_get_alphabet_id(null_ctx) != nullptr);
    dasher_set_alphabet_id(null_ctx, "test");
    ASSERT_EQ(dasher_get_language_model_id(null_ctx), 0);
    dasher_set_language_model_id(null_ctx, 1);
    ASSERT_EQ(dasher_get_speed_percent(null_ctx), 100);
    dasher_set_speed_percent(null_ctx, 150);
    ASSERT_EQ(dasher_get_bool_parameter(null_ctx, 0), 0);
    dasher_set_bool_parameter(null_ctx, 0, 1);
    ASSERT_EQ(dasher_get_long_parameter(null_ctx, 0), 0);
    dasher_set_long_parameter(null_ctx, 0, 100);
    ASSERT(dasher_get_string_parameter(null_ctx, 0) != nullptr);
    dasher_set_string_parameter(null_ctx, 0, "test");

    printf("✓ null_safety passed\n");
}

TEST(locale) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Default locale is "en"
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    // Get English parameter name (default)
    int param_count = dasher_get_parameter_count();
    ASSERT(param_count > 0);

    dasher_parameter_info info;
    ASSERT_EQ(dasher_get_parameter_info(0, &info), 0);
    const char* en_name = info.name;
    ASSERT(en_name != nullptr);
    ASSERT(strlen(en_name) > 0);
    printf("  English param name: '%s'\n", en_name);

    // Set locale to German
    int result = dasher_set_locale(ctx, "de");
    if (result == 0) {
        ASSERT_STR_EQ(dasher_get_locale(ctx), "de");

        // Same parameter should now return German name
        ASSERT_EQ(dasher_get_parameter_info(0, &info), 0);
        printf("  German param name: '%s'\n", info.name);
        // Should be different from English (unless untranslated)
        // At minimum, it should not be empty
        ASSERT(info.name != nullptr);
        ASSERT(strlen(info.name) > 0);
    } else {
        printf("  ⚠ German locale file not found (expected if Strings/ not in data dir)\n");
    }

    // Reset to English
    ASSERT_EQ(dasher_set_locale(ctx, "en"), 0);
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    // Test null locale resets to English
    ASSERT_EQ(dasher_set_locale(ctx, nullptr), 0);
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    // Test invalid locale returns -1
    ASSERT_EQ(dasher_set_locale(ctx, "xx_INVALID"), -1);

    // Test get_localized_string
    const char* localized = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    (void)localized;

    dasher_destroy(ctx);
    printf("✓ locale passed\n");
}

TEST(string_override) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Override a string
    dasher_set_string_override(ctx, "BP_DRAW_MOUSE_LINE.label", "My Custom Label");

    const char* val = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    ASSERT(val != nullptr);
    ASSERT_STR_EQ(val, "My Custom Label");

    // Override should appear in parameter info
    dasher_parameter_info info;
    int param_count = dasher_get_parameter_count();
    bool found = false;
    (void)found;
    for (int i = 0; i < param_count; i++) {
        dasher_get_parameter_info(i, &info);
        if (info.key == 0) { // BP_DRAW_MOUSE_LINE is key 0 in enum
            // Note: key might not be 0, search by checking
        }
    }
    // More reliably: set override and check via localized string
    ASSERT_STR_EQ(dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label"), "My Custom Label");

    // Clear override
    dasher_set_string_override(ctx, "BP_DRAW_MOUSE_LINE.label", nullptr);
    val = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    ASSERT(val == nullptr); // Should be gone, back to built-in

    dasher_destroy(ctx);
    printf("✓ string_override passed\n");
}

TEST(locale_multiple_languages) {
    const char* data_dir = get_test_data_dir();
    (void)data_dir;
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    const char* locales[] = {"de", "fr", "zh-CN", "ar"};
    int num_locales = 4;
    int loaded = 0;

    for (int i = 0; i < num_locales; i++) {
        int result = dasher_set_locale(ctx, locales[i]);
        if (result == 0) {
            loaded++;
            ASSERT_STR_EQ(dasher_get_locale(ctx), locales[i]);

            // Check that parameter info doesn't crash
            dasher_parameter_info info;
            int count = dasher_get_parameter_count();
            for (int j = 0; j < count && j < 5; j++) {
                ASSERT_EQ(dasher_get_parameter_info(j, &info), 0);
                ASSERT(info.name != nullptr);
                ASSERT(info.desc != nullptr);
            }
        }
    }

    printf("  Loaded %d/%d locale files\n", loaded, num_locales);
    ASSERT(loaded > 0); // At least one should load if Strings/ is present

    dasher_destroy(ctx);
    printf("✓ locale_multiple_languages passed\n");
}

TEST(clipboard_callback) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Clipboard callback should be receivable
    static const char* lastClipText = nullptr;
    static int clipCallCount = 0;
    lastClipText = nullptr;
    clipCallCount = 0;

    auto cb = [](const char* text, void* user_data) {
        (void)user_data;
        lastClipText = text;
        clipCallCount++;
    };

    dasher_set_clipboard_callback(ctx, cb, nullptr);

    // We can't easily trigger a copy from the test without a running engine frame loop,
    // but we can verify the callback registration doesn't crash and SupportsClipboard
    // is now true (indirectly verified by no segfault).
    // A real copy would happen via alphabet copyToClipboardAction during dasher_frame.
    ASSERT(clipCallCount == 0);

    // Unregister
    dasher_set_clipboard_callback(ctx, nullptr, nullptr);

    dasher_destroy(ctx);
    printf("✓ clipboard_callback passed\n");
}

TEST(output_text_reset) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Initially empty
    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    ASSERT_EQ((int)strlen(text), 0);

    // Reset should keep it empty
    dasher_reset_output_text(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    ASSERT_EQ((int)strlen(text), 0);

    // Full reset
    dasher_reset(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    ASSERT_EQ((int)strlen(text), 0);

    dasher_destroy(ctx);
    printf("✓ output_text_reset passed\n");
}

int main(int argc, char* argv[]) {
    printf("Running Dasher C API tests...\n\n");

    // Run all tests
    test_color_utilities();
    test_context_creation();
    test_screen_size();
    test_parameters();
    test_output_text();
    test_alphabet();
    test_null_safety();
    test_locale();
    test_string_override();
    test_locale_multiple_languages();
    test_clipboard_callback();
    test_output_text_reset();

    printf("\n✓ All tests passed!\n");
    return 0;
}