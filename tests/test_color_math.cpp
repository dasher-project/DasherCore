// Color math tests: verify ARGB construction, extraction, and round-trips
#include "test_common.h"

TEST(color_argb_construction) {
    int c = dasher_color_argb(255, 128, 64, 32);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 128);
    ASSERT_EQ(dasher_color_get_green(c), 64);
    ASSERT_EQ(dasher_color_get_blue(c), 32);
    printf("  ARGB(255,128,64,32) = 0x%08X\n", c);
}

TEST(color_rgb_is_opaque) {
    int c = dasher_color_rgb(200, 100, 50);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 200);
    ASSERT_EQ(dasher_color_get_green(c), 100);
    ASSERT_EQ(dasher_color_get_blue(c), 50);
}

TEST(color_alpha_zero) {
    int c = dasher_color_argb(0, 255, 255, 255);
    ASSERT_EQ(dasher_color_get_alpha(c), 0);
    ASSERT_EQ(dasher_color_get_red(c), 255);
    ASSERT_EQ(dasher_color_get_green(c), 255);
    ASSERT_EQ(dasher_color_get_blue(c), 255);
}

TEST(color_black) {
    int c = dasher_color_rgb(0, 0, 0);
    ASSERT_EQ(c, (int)0xFF000000);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 0);
    ASSERT_EQ(dasher_color_get_green(c), 0);
    ASSERT_EQ(dasher_color_get_blue(c), 0);
}

TEST(color_white) {
    int c = dasher_color_rgb(255, 255, 255);
    ASSERT_EQ(c, (int)0xFFFFFFFF);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 255);
    ASSERT_EQ(dasher_color_get_green(c), 255);
    ASSERT_EQ(dasher_color_get_blue(c), 255);
}

TEST(color_round_trip_many) {
    int test_colors[][4] = {
        {0, 0, 0, 0},      {255, 0, 0, 0},     {0, 255, 0, 0},       {0, 0, 255, 0}, {0, 0, 0, 255},
        {128, 64, 32, 16}, {200, 100, 50, 25}, {255, 255, 255, 255}, {1, 2, 3, 4},   {254, 253, 252, 251},
    };
    int n = sizeof(test_colors) / sizeof(test_colors[0]);

    for (int i = 0; i < n; i++) {
        int a = test_colors[i][0], r = test_colors[i][1];
        int g = test_colors[i][2], b = test_colors[i][3];
        int c = dasher_color_argb(a, r, g, b);
        ASSERT_EQ(dasher_color_get_alpha(c), a);
        ASSERT_EQ(dasher_color_get_red(c), r);
        ASSERT_EQ(dasher_color_get_green(c), g);
        ASSERT_EQ(dasher_color_get_blue(c), b);
    }
    printf("  %d colors round-tripped correctly\n", n);
}

TEST(color_palette_preview_nonempty) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int palette_count = dasher_get_palette_count(ctx);
    printf("  Palettes: %d\n", palette_count);
    ASSERT(palette_count > 0);

    for (int i = 0; i < palette_count && i < 5; i++) {
        const char* name = dasher_get_palette_name(ctx, i);
        int colors[4];
        int rc = dasher_get_palette_preview_colors(ctx, i, colors);
        ASSERT_EQ(rc, 0);
        printf("  Palette '%s': 0x%08X 0x%08X 0x%08X 0x%08X\n", name, colors[0], colors[1], colors[2], colors[3]);
    }

    dasher_destroy(ctx);
}

TEST(color_palette_switch) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    const char* orig_palette = dasher_get_current_palette(ctx);
    printf("  Current palette: '%s'\n", orig_palette);

    int palette_count = dasher_get_palette_count(ctx);
    if (palette_count > 1) {
        const char* new_name = dasher_get_palette_name(ctx, 1);
        dasher_set_palette(ctx, new_name);
        const char* after = dasher_get_current_palette(ctx);
        printf("  Switched to: '%s'\n", after);
        ASSERT_STR_EQ(after, new_name);
    }

    dasher_destroy(ctx);
}

TEST(color_palette_appearance_classification) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int palette_count = dasher_get_palette_count(ctx);
    ASSERT(palette_count > 0);

    // Locate "Rainbow" (legacy light, no metadata) and "Rainbow Dark" (modern, dark).
    int rainbow_idx = -1, rainbow_dark_idx = -1;
    for (int i = 0; i < palette_count; i++) {
        const char* name = dasher_get_palette_name(ctx, i);
        if (strcmp(name, "Rainbow") == 0) rainbow_idx = i;
        if (strcmp(name, "Rainbow Dark") == 0) rainbow_dark_idx = i;
    }
    ASSERT_NEQ(rainbow_idx, -1);
    ASSERT_NEQ(rainbow_dark_idx, -1);

    // Legacy Rainbow carries no metadata -> unspecified (0).
    ASSERT_EQ(dasher_get_palette_appearance(ctx, rainbow_idx), 0);
    // Rainbow Dark declares appearance="dark" -> 2.
    ASSERT_EQ(dasher_get_palette_appearance(ctx, rainbow_dark_idx), 2);
    // Out-of-range index -> -1.
    ASSERT_EQ(dasher_get_palette_appearance(ctx, palette_count), -1);

    dasher_destroy(ctx);
}

TEST(color_palette_companion_lookup) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // Bidirectional: Rainbow (legacy, no metadata) resolves to Rainbow Dark via the
    // reverse scan; Rainbow Dark resolves to Rainbow via its explicit companion.
    const char* comp_of_rainbow = dasher_find_companion_palette(ctx, "Rainbow");
    ASSERT(comp_of_rainbow);
    ASSERT_STR_EQ(comp_of_rainbow, "Rainbow Dark");

    const char* comp_of_dark = dasher_find_companion_palette(ctx, "Rainbow Dark");
    ASSERT(comp_of_dark);
    ASSERT_STR_EQ(comp_of_dark, "Rainbow");

    // A palette with no companion (Yellow on Blue is inherently dark) returns NULL.
    const char* none = dasher_find_companion_palette(ctx, "Yellow on Blue");
    ASSERT(none == nullptr);

    dasher_destroy(ctx);
}

TEST(color_palette_appearance_mode_system) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // Fresh: mode defaults to SYSTEM (0), system appearance to LIGHT (1).
    ASSERT_EQ(dasher_get_appearance_mode(ctx), 0);
    ASSERT_EQ(dasher_get_system_appearance(ctx), 1);

    // Picker sets the user palette. In SYSTEM+light the light side is "Rainbow";
    // the dark side defaults to its companion "Rainbow Dark".
    dasher_set_user_palette(ctx, "Rainbow");
    ASSERT_STR_EQ(dasher_get_light_palette(ctx), "Rainbow");
    ASSERT_STR_EQ(dasher_get_dark_palette(ctx), "Rainbow Dark");
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");

    // OS goes dark -> active switches to the dark preference.
    dasher_set_system_appearance(ctx, 2);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow Dark");
    // The light preference is NOT clobbered by the auto-switch (the bug fix).
    ASSERT_STR_EQ(dasher_get_light_palette(ctx), "Rainbow");

    // OS goes light -> back to Rainbow.
    dasher_set_system_appearance(ctx, 1);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");

    dasher_destroy(ctx);
}

TEST(color_palette_appearance_forced_mode) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_user_palette(ctx, "Rainbow"); // light=Rainbow, dark=Rainbow Dark
    dasher_set_system_appearance(ctx, 2);    // system dark

    // Force LIGHT even though system is dark.
    dasher_set_appearance_mode(ctx, 1);
    ASSERT_EQ(dasher_get_appearance_mode(ctx), 1);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");

    // Force DARK.
    dasher_set_appearance_mode(ctx, 2);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow Dark");

    // Back to SYSTEM follows the (still dark) system input.
    dasher_set_appearance_mode(ctx, 0);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow Dark");

    // Invalid mode is rejected.
    dasher_set_appearance_mode(ctx, 9);
    ASSERT_EQ(dasher_get_appearance_mode(ctx), 0);

    dasher_destroy(ctx);
}

TEST(color_palette_appearance_independent_prefs) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    // The user may mix: Rainbow for light, TurboLUT Dark for dark (not 1:1).
    dasher_set_light_palette(ctx, "Rainbow");
    dasher_set_dark_palette(ctx, "TurboLUT Dark");

    dasher_set_appearance_mode(ctx, 1); // light
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");
    dasher_set_appearance_mode(ctx, 2); // dark
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "TurboLUT Dark");

    dasher_destroy(ctx);
}

TEST(color_palette_appearance_persistence) {
    // Two contexts sharing one user dir prove the user's explicit choice
    // survives an auto-switch across restarts (the persistence-bug regression).
    char sharedDir[256];
    snprintf(sharedDir, sizeof(sharedDir), "%s/dasher_persist_%d_%d", dasher_temp_dir(), dasher_getpid(),
             (int)(uintptr_t)sharedDir);
    dasher_mkdir(sharedDir);

    // Session 1: user picks Rainbow (light); system goes dark so active becomes
    // Rainbow Dark at close.
    dasher_ctx* c1 = dasher_create(TEST_DATA_DIR, sharedDir, nullptr);
    ASSERT(c1);
    dasher_set_screen_size(c1, 800, 600);
    dasher_set_user_palette(c1, "Rainbow");
    dasher_set_system_appearance(c1, 2);
    ASSERT_STR_EQ(dasher_get_current_palette(c1), "Rainbow Dark");
    dasher_save_settings(c1);
    dasher_destroy(c1);

    // Session 2: preferences must reload; active must re-resolve to the light
    // preference (system input is transient and defaults to light), NOT stay
    // stuck on the leaked "Rainbow Dark".
    dasher_ctx* c2 = dasher_create(TEST_DATA_DIR, sharedDir, nullptr);
    ASSERT(c2);
    dasher_set_screen_size(c2, 800, 600);
    ASSERT_STR_EQ(dasher_get_light_palette(c2), "Rainbow");
    ASSERT_STR_EQ(dasher_get_dark_palette(c2), "Rainbow Dark");
    ASSERT_STR_EQ(dasher_get_current_palette(c2), "Rainbow"); // original choice restored
    dasher_destroy(c2);
}
