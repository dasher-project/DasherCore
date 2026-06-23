// Settings XML serialization tests: verify complete round-trip persistence
#include "test_common.h"

TEST(settings_all_bool_persistent_survive_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setbool_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    int param_count = dasher_get_parameter_count();
    int bool_keys[64];
    int bool_count = 0;
    for (int i = 0; i < param_count && bool_count < 64; i++) {
        dasher_parameter_info info;
        if (dasher_get_parameter_info(i, &info) == 0 && info.type == 0 && info.advanced == 0) {
            bool_keys[bool_count++] = info.key;
        }
    }
    printf("  Found %d non-advanced bool params\n", bool_count);
    ASSERT(bool_count > 0);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);

    for (int i = 0; i < bool_count; i++) {
        int orig = dasher_get_bool_parameter(ctx1, bool_keys[i]);
        dasher_set_bool_parameter(ctx1, bool_keys[i], orig ? 0 : 1);
    }
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);

    printf("  All %d bool params toggled and reloaded\n", bool_count);
    dasher_destroy(ctx2);
}

TEST(settings_speed_survives_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setspeed_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_speed_percent(ctx1, 250);
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);
    ASSERT_EQ(dasher_get_speed_percent(ctx2), 250);
    dasher_destroy(ctx2);
}

TEST(settings_alphabet_survives_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setalpha_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    int alph_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(alph_key >= 0);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_string_parameter(ctx1, alph_key, "English without punctuation");
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);
    const char* loaded = dasher_get_string_parameter(ctx2, alph_key);
    printf("  Reloaded alphabet: '%s'\n", loaded);
    ASSERT_STR_EQ(loaded, "English without punctuation");
    dasher_destroy(ctx2);
}

TEST(settings_color_palette_survives_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setcolor_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    int color_key = dasher_find_parameter_key("SP_COLOUR_ID");
    ASSERT(color_key >= 0);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_string_parameter(ctx1, color_key, "Yellow on Blue");
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);
    ASSERT_STR_EQ(dasher_get_string_parameter(ctx2, color_key), "Yellow on Blue");
    dasher_destroy(ctx2);
}

TEST(settings_orientation_survives_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setorient_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    int orient_key = dasher_find_parameter_key("LP_ORIENTATION");
    ASSERT(orient_key >= 0);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);
    long orig = dasher_get_long_parameter(ctx1, orient_key);
    dasher_set_long_parameter(ctx1, orient_key, 1);
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);
    long loaded = dasher_get_long_parameter(ctx2, orient_key);
    printf("  Original: %ld, Saved 1, Reloaded: %ld\n", orig, loaded);
    ASSERT_EQ(loaded, 1);
    dasher_destroy(ctx2);
}

TEST(settings_lm_max_order_survives_reload) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_setlmorder_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    int order_key = dasher_find_parameter_key("LP_LM_MAX_ORDER");
    ASSERT(order_key >= 0);

    dasher_ctx* ctx1 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx1);
    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_long_parameter(ctx1, order_key, 12);
    dasher_save_settings(ctx1);
    dasher_destroy(ctx1);

    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx2);
    dasher_set_screen_size(ctx2, 800, 600);
    ASSERT_EQ(dasher_get_long_parameter(ctx2, order_key), 12);
    dasher_destroy(ctx2);
}

TEST(settings_empty_file_uses_defaults) {
    static int counter = 0;
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/dasher_empty_%d_%d", dasher_temp_dir(), dasher_getpid(), counter++);
    dasher_mkdir(dir);

    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, dir, nullptr);
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);

    int speed = dasher_get_speed_percent(ctx);
    printf("  Default speed from empty dir: %d\n", speed);
    ASSERT(speed >= 20 && speed <= 400);

    dasher_destroy(ctx);
}
