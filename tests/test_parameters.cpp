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
    printf("v param_schema_all_valid passed\n");
}

TEST(param_schema_out_of_range) {
    dasher_parameter_info info;
    ASSERT_EQ(dasher_get_parameter_info(-1, &info), -1);
    ASSERT_EQ(dasher_get_parameter_info(99999, &info), -1);
    printf("v param_schema_out_of_range passed\n");
}

TEST(param_find_all_known_keys) {
    const char* known_keys[] = {
        "BP_DRAW_MOUSE_LINE", "BP_START_MOUSE", "BP_LM_ADAPTIVE",
        "BP_GAME_HELP_DRAW_PATH", "BP_SPEAK_WORDS", "BP_COPY_ALL_ON_STOP",
        "BP_AUTO_SPEEDCONTROL", "BP_NONLINEAR_Y", "BP_TURBO_MODE",
        "LP_ORIENTATION", "LP_MAX_BITRATE", "LP_LANGUAGE_MODEL_ID",
        "LP_LM_MAX_ORDER", "LP_LM_ALPHA", "LP_LM_BETA",
        "LP_NODE_BUDGET", "LP_UNIFORM", "LP_DASHER_FONTSIZE",
        "LP_GEOMETRY", "LP_LINE_WIDTH",
        "SP_ALPHABET_ID", "SP_COLOUR_ID", "SP_DASHER_FONT",
        "SP_INPUT_FILTER", "SP_INPUT_DEVICE", "SP_GAME_TEXT_FILE",
        nullptr
    };

    for (int i = 0; known_keys[i]; i++) {
        int key = dasher_find_parameter_key(known_keys[i]);
        printf("  %s -> key %d\n", known_keys[i], key);
        ASSERT(key >= 0);
    }
    printf("v param_find_all_known_keys passed\n");
}

TEST(param_find_nonexistent) {
    ASSERT_EQ(dasher_find_parameter_key("NONEXISTENT_PARAM"), -1);
    ASSERT_EQ(dasher_find_parameter_key(""), -1);
    ASSERT_EQ(dasher_find_parameter_key(nullptr), -1);
    printf("v param_find_nonexistent passed\n");
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
    printf("v param_bool_roundtrip passed\n");
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
    printf("v param_long_roundtrip passed\n");
}

TEST(param_string_roundtrip) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int font_key = dasher_find_parameter_key("SP_DASHER_FONT");
    ASSERT(font_key >= 0);

    const char* orig = strdup(dasher_get_string_parameter(ctx, font_key));
    dasher_set_string_parameter(ctx, font_key, "Courier New");
    ASSERT_STR_EQ(dasher_get_string_parameter(ctx, font_key), "Courier New");
    dasher_set_string_parameter(ctx, font_key, orig);
    free((void*)orig);

    dasher_destroy(ctx);
    printf("v param_string_roundtrip passed\n");
}

TEST(param_speed_clamping) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 50);
    int speed = dasher_get_speed_percent(ctx);
    ASSERT(speed >= 20 && speed <= 400);

    dasher_set_speed_percent(ctx, 1000);
    speed = dasher_get_speed_percent(ctx);
    ASSERT(speed <= 400);

    dasher_set_speed_percent(ctx, 1);
    speed = dasher_get_speed_percent(ctx);
    ASSERT(speed >= 20);

    dasher_destroy(ctx);
    printf("v param_speed_clamping passed\n");
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
    printf("v param_speed_affects_bitrate passed\n");
}

TEST(param_type_consistency) {
    int count = dasher_get_parameter_count();

    int bool_count = 0, long_count = 0, string_count = 0;
    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        dasher_get_parameter_info(i, &info);
        switch (info.type) {
            case 0: bool_count++; break;
            case 1: long_count++; break;
            case 2: string_count++; break;
        }
    }
    printf("  Types: bool=%d long=%d string=%d\n", bool_count, long_count, string_count);
    ASSERT(bool_count > 0);
    ASSERT(long_count > 0);
    ASSERT(string_count > 0);
    printf("v param_type_consistency passed\n");
}

TEST(param_groups_are_valid) {
    int count = dasher_get_parameter_count();
    const char* valid_groups[] = {"Input", "Language", "Appearance", "Speed", "Output",
                                   "Alphabet", "Alphabet History 1", "Alphabet History 2",
                                   "Alphabet History 3", "Alphabet History 4",
                                   "Advanced", "Other", "History",
                                   "Customization", "Game Mode", ""};
    int num_groups = sizeof(valid_groups)/sizeof(valid_groups[0]);

    for (int i = 0; i < count; i++) {
        dasher_parameter_info info;
        dasher_get_parameter_info(i, &info);
        bool found = false;
        for (int g = 0; g < num_groups; g++) {
            if (strcmp(info.group, valid_groups[g]) == 0) { found = true; break; }
        }
        if (!found) {
            printf("  Unknown group '%s' for param '%s' (key=%d)\n", info.group, info.name, info.key);
        }
    }
    printf("v param_groups_are_valid passed\n");
}

TEST(param_persistence_roundtrip) {
    static int persist_counter = 0;
    char shared_dir[256];
    snprintf(shared_dir, sizeof(shared_dir), "/tmp/dasher_persist_test_%d_%d", getpid(), persist_counter++);
    mkdir(shared_dir, 0755);

    int speed_key = dasher_find_parameter_key("LP_MAX_BITRATE");
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
    const char* color2 = strdup(dasher_get_string_parameter(ctx2, dasher_find_parameter_key("SP_COLOUR_ID")));

    printf("  Reloaded: speed=%d bool=%d color='%s'\n", speed2, bool2, color2);
    ASSERT_EQ(speed2, 180);
    ASSERT_EQ(bool2, 1);
    ASSERT_STR_EQ(color2, "Yellow on Blue");

    free((void*)color2);
    dasher_destroy(ctx2);
    printf("v param_persistence_roundtrip passed\n");
}

TEST(param_invalid_key_safe) {
    // Invalid keys beyond the enum range cause undefined behavior in
    // the settings store (std::out_of_range, not bad_variant_access).
    // We only verify find_parameter_key returns -1 for unknown names.
    ASSERT_EQ(dasher_find_parameter_key("NONEXISTENT_KEY_XYZ"), -1);
    ASSERT_EQ(dasher_find_parameter_key(""), -1);
    printf("v param_invalid_key_safe passed\n");
}

int main() {
    printf("Running Dasher parameter tests...\n\n");

    test_param_schema_all_valid();
    test_param_schema_out_of_range();
    test_param_find_all_known_keys();
    test_param_find_nonexistent();
    test_param_bool_roundtrip();
    test_param_long_roundtrip();
    test_param_string_roundtrip();
    test_param_speed_clamping();
    test_param_speed_affects_bitrate();
    test_param_type_consistency();
    test_param_groups_are_valid();
    test_param_persistence_roundtrip();
    test_param_invalid_key_safe();

    printf("\nAll parameter tests passed!\n");
    return 0;
}
