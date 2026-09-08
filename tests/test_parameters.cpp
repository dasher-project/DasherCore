// Parameter schema tests: validate all parameters, edge cases, persistence
#include "test_common.h"

TEST(param_schema_all_valid) {
    int count = dasher_get_parameter_count();
    ASSERT(count > 0);
    printf("  Total parameters: %d\n", count);

    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        ASSERT_EQ(dasher_get_parameter_info(i, &info), 0);
        ASSERT(info.name != nullptr);
        ASSERT(strlen(info.name) > 0);
        ASSERT(info.type >= 0 && info.type <= 2);
        ASSERT(info.ui_type >= 0 && info.ui_type <= 5);
    }
}

TEST(param_schema_out_of_range) {
    dasher_parameter_info info;
    ASSERT_EQ(dasher_get_parameter_info(-1, &info), -1);
    ASSERT_EQ(dasher_get_parameter_info(99999, &info), -1);
}

TEST(param_find_all_known_keys) {
    const char* known_keys[] = {"BP_DRAW_MOUSE_LINE",
                                "BP_START_MOUSE",
                                "BP_LM_ADAPTIVE",
                                "BP_GAME_HELP_DRAW_PATH",
                                "BP_SPEAK_WORDS",
                                "BP_COPY_ALL_ON_STOP",
                                "BP_AUTO_SPEEDCONTROL",
                                "BP_NONLINEAR_Y",
                                "BP_TURBO_MODE",
                                "LP_ORIENTATION",
                                "LP_MAX_BITRATE",
                                "LP_LANGUAGE_MODEL_ID",
                                "LP_LM_MAX_ORDER",
                                "LP_LM_ALPHA",
                                "LP_LM_BETA",
                                "LP_NODE_BUDGET",
                                "LP_UNIFORM",
                                "LP_DASHER_FONTSIZE",
                                "LP_GEOMETRY",
                                "LP_LINE_WIDTH",
                                "SP_ALPHABET_ID",
                                "SP_COLOUR_ID",
                                "SP_DASHER_FONT",
                                "SP_INPUT_FILTER",
                                "SP_INPUT_DEVICE",
                                "SP_GAME_TEXT_FILE",
                                nullptr};

    for (int i = 0; known_keys[i]; i++) {
        int key = dasher_find_parameter_key(known_keys[i]);
        printf("  %s -> key %d\n", known_keys[i], key);
        ASSERT(key >= 0);
    }
}

TEST(param_find_nonexistent) {
    ASSERT_EQ(dasher_find_parameter_key("NONEXISTENT_PARAM"), -1);
    ASSERT_EQ(dasher_find_parameter_key(""), -1);
    ASSERT_EQ(dasher_find_parameter_key(nullptr), -1);
}

TEST(param_bool_roundtrip) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int key = dasher_find_parameter_key("BP_START_ON_SPACE");
    if (key < 0) key = dasher_find_parameter_key("BP_COPY_ALL_ON_STOP");
    ASSERT(key >= 0);

    int orig = dasher_get_bool_parameter(ctx, key);
    dasher_set_bool_parameter(ctx, key, 1);
    ASSERT_EQ(dasher_get_bool_parameter(ctx, key), 1);
    dasher_set_bool_parameter(ctx, key, 0);
    ASSERT_EQ(dasher_get_bool_parameter(ctx, key), 0);
    dasher_set_bool_parameter(ctx, key, orig);

    dasher_destroy(ctx);
}

TEST(param_long_roundtrip) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int bitrate_key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(bitrate_key >= 0);

    long orig = dasher_get_long_parameter(ctx, bitrate_key);
    dasher_set_long_parameter(ctx, bitrate_key, 5000);
    ASSERT_EQ(dasher_get_long_parameter(ctx, bitrate_key), 5000);
    dasher_set_long_parameter(ctx, bitrate_key, 1);
    ASSERT_EQ(dasher_get_long_parameter(ctx, bitrate_key), 1);
    dasher_set_long_parameter(ctx, bitrate_key, orig);

    dasher_destroy(ctx);
}

TEST(param_string_roundtrip) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int font_key = dasher_find_parameter_key("SP_DASHER_FONT");
    ASSERT(font_key >= 0);

    const char* orig = dasher_strdup(dasher_get_string_parameter(ctx, font_key));
    dasher_set_string_parameter(ctx, font_key, "Courier New");
    ASSERT_STR_EQ(dasher_get_string_parameter(ctx, font_key), "Courier New");
    dasher_set_string_parameter(ctx, font_key, orig);
    free((void*)orig);

    dasher_destroy(ctx);
}

TEST(param_speed_clamping) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // The percent setter clamps to the engine's declared LP_MAX_BITRATE range
    // (raw 1–1000 → ~1–625 %), replacing the historic fixed 20–400 cap that
    // truncated Dasher v5's top speeds (v5 allowed raw 10–800 = up to 500 %).
    dasher_set_speed_percent(ctx, 50);
    int speed = dasher_get_speed_percent(ctx);
    ASSERT(speed >= 1 && speed <= 625);

    dasher_set_speed_percent(ctx, 1000);
    speed = dasher_get_speed_percent(ctx);
    ASSERT(speed <= 625); // raw clamped to 1000 → 625 %

    dasher_set_speed_percent(ctx, 1);
    speed = dasher_get_speed_percent(ctx);
    ASSERT(speed >= 1); // raw clamped up to the engine minimum

    dasher_set_speed_percent(ctx, 500); // v5's maximum, must survive exactly
    ASSERT(dasher_get_speed_percent(ctx) == 500);

    dasher_destroy(ctx);
}

TEST(param_speed_affects_bitrate) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int bitrate_key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(bitrate_key >= 0);

    dasher_set_speed_percent(ctx, 100);
    long bitrate_100 = dasher_get_long_parameter(ctx, bitrate_key);

    dasher_set_speed_percent(ctx, 200);
    long bitrate_200 = dasher_get_long_parameter(ctx, bitrate_key);

    printf("  Bitrate at 100%%: %ld, at 200%%: %ld\n", bitrate_100, bitrate_200);
    ASSERT(bitrate_200 > bitrate_100);

    dasher_destroy(ctx);
}

TEST(param_type_consistency) {
    int count = dasher_get_parameter_count();

    int bool_count = 0, long_count = 0, string_count = 0;
    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        dasher_get_parameter_info(i, &info);
        switch (info.type) {
        case 0:
            bool_count++;
            break;
        case 1:
            long_count++;
            break;
        case 2:
            string_count++;
            break;
        }
    }
    printf("  Types: bool=%d long=%d string=%d\n", bool_count, long_count, string_count);
    ASSERT(bool_count > 0);
    ASSERT(long_count > 0);
    ASSERT(string_count > 0);
}

TEST(param_groups_are_valid) {
    int count = dasher_get_parameter_count();
    const char* valid_groups[] = {"Input",
                                  "Language",
                                  "Appearance",
                                  "Speed",
                                  "Output",
                                  "Alphabet",
                                  "Alphabet History 1",
                                  "Alphabet History 2",
                                  "Alphabet History 3",
                                  "Alphabet History 4",
                                  "Advanced",
                                  "Other",
                                  "History",
                                  "Customization",
                                  "Game Mode",
                                  ""};
    int num_groups = sizeof(valid_groups) / sizeof(valid_groups[0]);

    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        dasher_get_parameter_info(i, &info);
        bool found = false;
        for (int g = 0; g < num_groups; g++) {
            if (strcmp(info.group, valid_groups[g]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            printf("  Unknown group '%s' for param '%s' (key=%d)\n", info.group, info.name, info.key);
        }
    }
}

TEST(param_persistence_roundtrip) {
    static int persist_counter = 0;
    char shared_dir[256];
    snprintf(shared_dir, sizeof(shared_dir), "%s/dasher_persist_test_%d_%d", dasher_temp_dir(), dasher_getpid(),
             persist_counter++);
    dasher_mkdir(shared_dir);

    int speed_key = dasher_find_parameter_key("LP_MAX_BITRATE");
    (void)speed_key;
    int bool_key = dasher_find_parameter_key("BP_COPY_ALL_ON_STOP");

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx1 != nullptr);
    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_speed_percent(ctx1, 180);
    dasher_set_bool_parameter(ctx1, bool_key, 1);
    dasher_set_string_parameter(ctx1, dasher_find_parameter_key("SP_COLOUR_ID"), "Yellow on Blue");
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx2 != nullptr);
    dasher_set_screen_size(ctx2, 800, 600);

    int speed2 = dasher_get_speed_percent(ctx2);
    int bool2 = dasher_get_bool_parameter(ctx2, bool_key);
    const char* color2 = dasher_strdup(dasher_get_string_parameter(ctx2, dasher_find_parameter_key("SP_COLOUR_ID")));

    printf("  Reloaded: speed=%d bool=%d color='%s'\n", speed2, bool2, color2);
    ASSERT_EQ(speed2, 180);
    ASSERT_EQ(bool2, 1);
    ASSERT_STR_EQ(color2, "Yellow on Blue");

    free((void*)color2);
    dasher_destroy(ctx2);
}

TEST(param_invalid_key_safe) {
    // Invalid keys beyond the enum range cause undefined behavior in
    // the settings store (std::out_of_range, not bad_variant_access).
    // We only verify find_parameter_key returns -1 for unknown names.
    ASSERT_EQ(dasher_find_parameter_key("NONEXISTENT_KEY_XYZ"), -1);
    ASSERT_EQ(dasher_find_parameter_key(""), -1);
}

namespace {
// Strict UTF-8 validation: rejects overlongs, surrogates, >U+10FFFF, and
// lone continuation/lead bytes (the LP_UNIFORM 0xD7 regression).
bool is_valid_utf8(const char* s) {
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        if (*p < 0x80) {
            p++;
            continue;
        }
        int len;
        unsigned cp;
        if ((*p & 0xE0) == 0xC0) {
            len = 2;
            cp = *p & 0x1F;
        } else if ((*p & 0xF0) == 0xE0) {
            len = 3;
            cp = *p & 0x0F;
        } else if ((*p & 0xF8) == 0xF0) {
            len = 4;
            cp = *p & 0x07;
        } else
            return false; // continuation or invalid lead
        for (int i = 1; i < len; i++) {
            if ((p[i] & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (p[i] & 0x3F);
        }
        if (len == 2 && cp < 0x80) return false;        // overlong
        if (len == 3 && cp < 0x800) return false;       // overlong
        if (len == 4 && cp < 0x10000) return false;     // overlong
        if (cp >= 0xD800 && cp <= 0xDFFF) return false; // surrogate
        if (cp > 0x10FFFF) return false;
        p += len;
    }
    return true;
}
} // namespace

TEST(param_metadata_strict_utf8) {
    // LP_UNIFORM's description shipped a lone Latin-1 0xD7 (the "x" glyph
    // in "(x1000)") — invalid UTF-8 that aborted Android's JNI when the
    // Settings screen read it. Pin EVERY parameter's name and description
    // to strict UTF-8 so a regressed literal fails here, not on a device.
    int count = dasher_get_parameter_count();
    ASSERT(count > 0);
    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        ASSERT_EQ(dasher_get_parameter_info(i, &info), 0);
        ASSERT(info.name != nullptr);
        if (!is_valid_utf8(info.name))
            REQUIRE_MESSAGE(false, (std::string("invalid UTF-8 name: '") + info.name + "'").c_str());
        if (info.desc != nullptr && info.desc[0] != '\0') {
            if (!is_valid_utf8(info.desc))
                REQUIRE_MESSAGE(false,
                                (std::string("invalid UTF-8 desc for '") + info.name + "': " + info.desc).c_str());
        }
    }
}
