// Color math tests: verify ARGB construction, extraction, and round-trips
#include "test_common.h"

TEST(color_argb_construction) {
    int c = dasher_color_argb(255, 128, 64, 32);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 128);
    ASSERT_EQ(dasher_color_get_green(c), 64);
    ASSERT_EQ(dasher_color_get_blue(c), 32);
    printf("  ARGB(255,128,64,32) = 0x%08X\n", c);
    printf("v color_argb_construction passed\n");
}

TEST(color_rgb_is_opaque) {
    int c = dasher_color_rgb(200, 100, 50);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 200);
    ASSERT_EQ(dasher_color_get_green(c), 100);
    ASSERT_EQ(dasher_color_get_blue(c), 50);
    printf("v color_rgb_is_opaque passed\n");
}

TEST(color_alpha_zero) {
    int c = dasher_color_argb(0, 255, 255, 255);
    ASSERT_EQ(dasher_color_get_alpha(c), 0);
    ASSERT_EQ(dasher_color_get_red(c), 255);
    ASSERT_EQ(dasher_color_get_green(c), 255);
    ASSERT_EQ(dasher_color_get_blue(c), 255);
    printf("v color_alpha_zero passed\n");
}

TEST(color_black) {
    int c = dasher_color_rgb(0, 0, 0);
    ASSERT_EQ(c, (int)0xFF000000);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 0);
    ASSERT_EQ(dasher_color_get_green(c), 0);
    ASSERT_EQ(dasher_color_get_blue(c), 0);
    printf("v color_black passed\n");
}

TEST(color_white) {
    int c = dasher_color_rgb(255, 255, 255);
    ASSERT_EQ(c, (int)0xFFFFFFFF);
    ASSERT_EQ(dasher_color_get_alpha(c), 255);
    ASSERT_EQ(dasher_color_get_red(c), 255);
    ASSERT_EQ(dasher_color_get_green(c), 255);
    ASSERT_EQ(dasher_color_get_blue(c), 255);
    printf("v color_white passed\n");
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
    printf("v color_round_trip_many passed\n");
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
    printf("v color_palette_preview_nonempty passed\n");
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
    printf("v color_palette_switch passed\n");
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
    printf("v color_palette_appearance_classification passed\n");
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
    printf("v color_palette_companion_lookup passed\n");
}

TEST(color_palette_set_appearance) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_palette(ctx, "Rainbow");
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");

    // Request dark -> switch to Rainbow Dark.
    ASSERT_EQ(dasher_set_appearance(ctx, 2), 0);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow Dark");

    // Requesting dark again is a no-op (already on the dark variant).
    ASSERT_EQ(dasher_set_appearance(ctx, 2), 0);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow Dark");

    // Request light -> switch back to Rainbow.
    ASSERT_EQ(dasher_set_appearance(ctx, 1), 0);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Rainbow");

    // A palette with no companion cannot switch -> -1, and is left unchanged.
    dasher_set_palette(ctx, "Yellow on Blue");
    ASSERT_EQ(dasher_set_appearance(ctx, 1), -1);
    ASSERT_STR_EQ(dasher_get_current_palette(ctx), "Yellow on Blue");

    // Invalid appearance value -> -1.
    ASSERT_EQ(dasher_set_appearance(ctx, 9), -1);

    dasher_destroy(ctx);
    printf("v color_palette_set_appearance passed\n");
}

int main() {
    printf("Running color math tests...\n\n");

    test_color_argb_construction();
    test_color_rgb_is_opaque();
    test_color_alpha_zero();
    test_color_black();
    test_color_white();
    test_color_round_trip_many();
    test_color_palette_preview_nonempty();
    test_color_palette_switch();
    test_color_palette_appearance_classification();
    test_color_palette_companion_lookup();
    test_color_palette_set_appearance();

    printf("\nAll color math tests passed!\n");
    return 0;
}
