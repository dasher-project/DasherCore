// Basic tests for the Dasher C API
// These tests provide a contract that frontends can verify

#include "test_common.h"

#include <string>

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
}

TEST(context_creation) {
    // Previously this test passed whether the alphabet loaded or not — it just
    // printed a warning on empty. The contract is that a context, once given
    // a screen size, must load the default alphabet. (Realize happens lazily
    // on first frame; before that the alphabet id may be "".)
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    // Before set_screen_size: pointer is valid but may be empty.
    const char* alphabet_early = dasher_get_alphabet_id(ctx);
    REQUIRE(alphabet_early != nullptr);

    // After set_screen_size: default alphabet must be loaded.
    dasher_set_screen_size(ctx, 800, 600);
    const char* alphabet = dasher_get_alphabet_id(ctx);
    REQUIRE(alphabet != nullptr);
    CHECK(std::string(alphabet).size() > 0);
}

TEST(screen_size) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);

    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;

    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    // Should have some commands (at minimum clear screen)
    REQUIRE(commands != nullptr);
    CHECK(cmd_count > 0);
}

TEST(parameters) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    int speed = dasher_get_speed_percent(ctx);
    CHECK(speed >= 20);
    CHECK(speed <= 400);

    dasher_set_speed_percent(ctx, 150);
    speed = dasher_get_speed_percent(ctx);
    ASSERT_EQ(speed, 150);

    int model = dasher_get_language_model_id(ctx);
    CHECK(model >= 0);

    dasher_set_language_model_id(ctx, 2);
    model = dasher_get_language_model_id(ctx);
    ASSERT_EQ(model, 2);

    int start_on_space = dasher_get_bool_parameter(ctx, 0);
    CHECK((start_on_space == 0 || start_on_space == 1));

    dasher_set_bool_parameter(ctx, 0, 1);
    start_on_space = dasher_get_bool_parameter(ctx, 0);
    ASSERT_EQ(start_on_space, 1);
}

TEST(output_text) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);

    const char* text = dasher_get_output_text(ctx);
    REQUIRE(text != nullptr);
    CHECK(std::string(text).size() == 0);

    dasher_reset_output_text(ctx);
    text = dasher_get_output_text(ctx);
    CHECK(std::string(text).size() == 0);
}

TEST(alphabet) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* alphabet = dasher_get_alphabet_id(ctx);
    REQUIRE(alphabet != nullptr);
    CHECK(std::string(alphabet).size() > 0);

    std::string original(alphabet);
    dasher_set_alphabet_id(ctx, "English");
    dasher_set_alphabet_id(ctx, original.c_str());
    const char* new_alphabet = dasher_get_alphabet_id(ctx);
    ASSERT_STR_EQ(new_alphabet, original.c_str());
}

TEST(null_safety) {
    // All these calls must handle null ctx gracefully without crashing.
    // We don't assert specific return values here (those are covered by the
    // individual functional tests); the contract is just "no crash, no throw".
    dasher_ctx* null_ctx = nullptr;

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

    CHECK(dasher_get_output_text(null_ctx) != nullptr);
    dasher_reset_output_text(null_ctx);
    CHECK(dasher_get_alphabet_id(null_ctx) != nullptr);
    dasher_set_alphabet_id(null_ctx, "test");
    CHECK(dasher_get_language_model_id(null_ctx) == 0);
    dasher_set_language_model_id(null_ctx, 1);
    CHECK(dasher_get_speed_percent(null_ctx) == 100);
    dasher_set_speed_percent(null_ctx, 150);
    CHECK(dasher_get_bool_parameter(null_ctx, 0) == 0);
    dasher_set_bool_parameter(null_ctx, 0, 1);
    CHECK(dasher_get_long_parameter(null_ctx, 0) == 0);
    dasher_set_long_parameter(null_ctx, 0, 100);
    CHECK(dasher_get_string_parameter(null_ctx, 0) != nullptr);
    dasher_set_string_parameter(null_ctx, 0, "test");
}

TEST(locale) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    // Default locale is "en"
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    int param_count = dasher_get_parameter_count();
    CHECK(param_count > 0);

    dasher_parameter_info info;
    REQUIRE(dasher_get_parameter_info(0, &info) == 0);
    REQUIRE(info.name != nullptr);
    CHECK(std::string(info.name).size() > 0);

    // Set locale to German (skipped gracefully if Strings/de.json missing)
    if (dasher_set_locale(ctx, "de") == 0) {
        ASSERT_STR_EQ(dasher_get_locale(ctx), "de");
        REQUIRE(dasher_get_parameter_info(0, &info) == 0);
        REQUIRE(info.name != nullptr);
        CHECK(std::string(info.name).size() > 0);
    }

    // Reset to English
    REQUIRE(dasher_set_locale(ctx, "en") == 0);
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    // Null locale resets to English
    REQUIRE(dasher_set_locale(ctx, nullptr) == 0);
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    // Invalid locale returns -1
    CHECK(dasher_set_locale(ctx, "xx_INVALID") == -1);

    // get_localized_string should not crash on a known key
    const char* localized = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    (void)localized;
}

TEST(string_override) {
    // Previously this test had a `bool found = false; (void)found;` block
    // and a loop that did nothing useful. The real contract — that overrides
    // take effect immediately and can be cleared by passing null — is what
    // we assert here.
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    // Baseline: built-in English value (or empty if no built-in string).
    const char* before = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");

    // Override should take effect immediately.
    dasher_set_string_override(ctx, "BP_DRAW_MOUSE_LINE.label", "My Custom Label");
    const char* val = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    REQUIRE(val != nullptr);
    ASSERT_STR_EQ(val, "My Custom Label");

    // Clearing the override restores prior behavior. The exact returned
    // value depends on whether a built-in existed; we only require that
    // "My Custom Label" is no longer returned.
    dasher_set_string_override(ctx, "BP_DRAW_MOUSE_LINE.label", nullptr);
    const char* after = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    if (after != nullptr) {
        CHECK_NE(std::string(after), std::string("My Custom Label"));
    }
    (void)before;
}

TEST(locale_multiple_languages) {
    ScopedContext ctx;
    REQUIRE(ctx.ctx != nullptr);

    const char* locales[] = {"de", "fr", "zh-CN", "ar"};
    int loaded = 0;

    for (const char* loc : locales) {
        if (dasher_set_locale(ctx, loc) == 0) {
            ++loaded;
            ASSERT_STR_EQ(dasher_get_locale(ctx), loc);

            dasher_parameter_info info;
            int count = dasher_get_parameter_count();
            for (int j = 0; j < count && j < 5; j++) {
                REQUIRE(dasher_get_parameter_info(j, &info) == 0);
                REQUIRE(info.name != nullptr);
                REQUIRE(info.desc != nullptr);
            }
        }
    }

    CHECK(loaded > 0);  // at least one locale file must load when Strings/ is present
}
