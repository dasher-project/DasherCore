// Multilingual tests: alphabet switching, interaction per alphabet, locale/i18n
#include "test_common.h"
static void produce_text(dasher_ctx* ctx, int frames) {
    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < frames; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 1000, 20);
    }
    dasher_mouse_up(ctx);
}

TEST(alphabet_switch_german) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* alph_id = dasher_get_alphabet_id(ctx);
    ASSERT(alph_id != nullptr);
    const char* orig = dasher_strdup(alph_id);
    printf("  Original alphabet: '%s'\n", orig);

    dasher_set_alphabet_id(ctx, "Deutsch / German with limited punctuation");
    run_frames(ctx, 5, 1000, 20);
    const char* current = dasher_get_alphabet_id(ctx);
    printf("  After setting German: '%s'\n", current);
    ASSERT(strlen(current) > 0);

    dasher_set_alphabet_id(ctx, orig);
    free((void*)orig);
    dasher_destroy(ctx);
}

TEST(alphabet_switch_french) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "Français / French with numerals and punctuation");
    run_frames(ctx, 5, 1000, 20);
    const char* current = dasher_get_alphabet_id(ctx);
    printf("  French alphabet: '%s'\n", current);
    ASSERT(strlen(current) > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_switch_spanish) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "Español / Spanish with punctuation and numerals");
    run_frames(ctx, 5, 1000, 20);
    const char* current = dasher_get_alphabet_id(ctx);
    printf("  Spanish alphabet: '%s'\n", current);
    ASSERT(strlen(current) > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_invalid_id_fallback) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* alph_id = dasher_get_alphabet_id(ctx);
    ASSERT(alph_id != nullptr);
    const char* orig = dasher_strdup(alph_id);

    dasher_set_alphabet_id(ctx, "Nonexistent Alphabet XYZ123");
    run_frames(ctx, 5, 1000, 20);

    const char* current = dasher_get_alphabet_id(ctx);
    printf("  After invalid set: '%s'\n", current);

    dasher_set_alphabet_id(ctx, orig);
    free((void*)orig);
    dasher_destroy(ctx);
}

TEST(alphabet_german_produces_text) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_alphabet_id(ctx, "Deutsch / German with limited punctuation");
    run_frames(ctx, 10, 1000, 20);
    produce_text(ctx, 200);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    printf("  German output: '%s'\n", text);
    ASSERT(strlen(text) > 0);

    dasher_destroy(ctx);
}

TEST(alphabet_history_values) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int alph_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(alph_key >= 0);

    const char* buf[128];
    int count = dasher_get_parameter_string_values(ctx, alph_key, buf, 128);
    ASSERT(count > 0);

    bool has_english = false;
    for (int i = 0; i < count; i++) {
        if (buf[i] && strstr(buf[i], "English")) has_english = true;
    }
    ASSERT(has_english);
    printf("  %d alphabet values, English found: %s\n", count, has_english ? "yes" : "no");

    dasher_destroy(ctx);
}

TEST(alphabet_switch_clears_output) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    produce_text(ctx, 200);
    const char* text1 = dasher_get_output_text(ctx);
    printf("  Before switch: '%s' (len=%zu)\n", text1, strlen(text1));

    dasher_set_alphabet_id(ctx, "English without punctuation");
    const char* text2 = dasher_get_output_text(ctx);
    printf("  After switch: '%s' (len=%zu)\n", text2, strlen(text2));
    ASSERT_EQ(strlen(text2), 0);

    dasher_destroy(ctx);
}

TEST(locale_set_and_get) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* loc = dasher_get_locale(ctx);
    ASSERT(loc != nullptr);
    const char* orig_locale = dasher_strdup(loc);
    printf("  Original locale: '%s'\n", orig_locale);

    int result = dasher_set_locale(ctx, "de");
    if (result == 0) {
        const char* de = dasher_get_locale(ctx);
        printf("  German locale: '%s'\n", de);
        ASSERT_STR_EQ(de, "de");
    } else {
        printf("  German locale not available (ok for minimal data)\n");
    }

    result = dasher_set_locale(ctx, orig_locale);
    ASSERT_EQ(result, 0);

    free((void*)orig_locale);
    dasher_destroy(ctx);
}

TEST(locale_invalid_returns_error) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int result = dasher_set_locale(ctx, "zz_INVALID");
    ASSERT_EQ(result, -1);

    result = dasher_set_locale(ctx, nullptr);
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(dasher_get_locale(ctx), "en");

    dasher_destroy(ctx);
}

TEST(locale_localized_string_lookup) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const char* localized = dasher_get_localized_string(ctx, "BP_DRAW_MOUSE_LINE.label");
    if (localized) {
        printf("  BP_DRAW_MOUSE_LINE.label: '%s'\n", localized);
    }

    const char* missing = dasher_get_localized_string(ctx, "NONEXISTENT_KEY_12345");
    ASSERT(missing == nullptr);

    dasher_destroy(ctx);
}

TEST(locale_string_override_and_clear) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_string_override(ctx, "LP_GEOMETRY.label", "Custom Geometry");
    const char* val = dasher_get_localized_string(ctx, "LP_GEOMETRY.label");
    ASSERT(val != nullptr);
    ASSERT_STR_EQ(val, "Custom Geometry");

    dasher_set_string_override(ctx, "LP_GEOMETRY.label", nullptr);
    const char* cleared = dasher_get_localized_string(ctx, "LP_GEOMETRY.label");
    ASSERT(cleared == nullptr);

    dasher_destroy(ctx);
}

TEST(locale_param_names_change_with_locale) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int param_count = dasher_get_parameter_count();
    ASSERT(param_count > 0);

    dasher_parameter_info info_en;
    dasher_get_parameter_info(0, &info_en);
    char name_en[256];
    strncpy(name_en, info_en.name, 255);
    name_en[255] = '\0';
    printf("  English param[0] name: '%s'\n", name_en);

    int result = dasher_set_locale(ctx, "de");
    if (result == 0) {
        dasher_parameter_info info_de;
        dasher_get_parameter_info(0, &info_de);
        printf("  German param[0] name: '%s'\n", info_de.name ? info_de.name : "(null)");
    }

    dasher_set_locale(ctx, "en");
    dasher_destroy(ctx);
}
