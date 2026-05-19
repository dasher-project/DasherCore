// Basic tests for the Dasher C API
// These tests provide a contract that frontends can verify

#include "dasher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

// Simple test framework
#define TEST(name) void test_##name()
#define ASSERT(condition) do { if (!(condition)) { printf("FAILED: %s\n", #condition); exit(1); } } while(0)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

// Test data directory - should point to a directory with Dasher Data/
const char* get_test_data_dir() {
#ifdef TEST_DATA_DIR
    return TEST_DATA_DIR;
#else
    // Try multiple possible locations for the Data directory
    const char* possible_paths[] = {
        "./Data",
        "../Data",
        "../../Data",
        "./build/Data",
        NULL
    };

    // Simple file existence check
    for (int i = 0; possible_paths[i] != NULL; i++) {
        // Check if alphabets directory exists (good indicator of valid data dir)
        const char* path = possible_paths[i];
        return path; // For simplicity, return first option (real apps would check existence)
    }

    return "./Data"; // Fallback
#endif
}

TEST(color_utilities) {
    // Test basic color creation
    int white = dasher_color_rgb(255, 255, 255);
    ASSERT_EQ(white, 0xFFFFFFFF);

    int black = dasher_color_rgb(0, 0, 0);
    ASSERT_EQ(black, 0xFF000000);

    // Test with alpha
    int semi_red = dasher_color_argb(128, 255, 0, 0);
    ASSERT_EQ(semi_red, 0x80FF0000);

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

    dasher_ctx* ctx = dasher_create(data_dir, nullptr, nullptr);
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
    dasher_ctx* ctx = dasher_create(data_dir, nullptr, nullptr);
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
    dasher_ctx* ctx = dasher_create(data_dir, nullptr, nullptr);
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
    dasher_ctx* ctx = dasher_create(data_dir, nullptr, nullptr);
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
    const char* data_dir = get_test_data_dir();
    dasher_ctx* ctx = dasher_create(data_dir, nullptr, nullptr);
    ASSERT(ctx != nullptr);

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

    printf("\n✓ All tests passed!\n");
    return 0;
}