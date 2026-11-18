// Settings XML serialization tests: verify complete round-trip persistence
#include "test_common.h"

#include <memory>

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

// #118: a failed settings write must be reported through the message
// callback, and the pending changes must be retained and retried by a later
// save — without another parameter change. Recipe from the issue: save into
// a writable dir, make the destination unwritable, change a persistent
// parameter and save (fails), restore write access and save again.
TEST(settings_save_failure_reported_and_retried) {
    ScopedContext ctx(800, 600);
    ASSERT(ctx != nullptr);

    const std::filesystem::path settingsFile = std::filesystem::path(ctx.dir.path) / "dasher_settings.xml";

    // 1. Writable destination: a first save succeeds and creates the file.
    dasher_set_speed_percent(ctx, 250);
    dasher_save_settings(ctx);
    ASSERT(std::filesystem::is_regular_file(settingsFile));

    static int failure_messages = 0;
    static int failure_message_type = -1;
    failure_messages = 0;
    failure_message_type = -1;
    dasher_set_message_callback(
        ctx,
        [](int message_type, const char* text, void*) {
            if (text && strstr(text, "Unable to save settings") != nullptr) {
                failure_messages++;
                failure_message_type = message_type;
            }
        },
        nullptr);

    // 2. Make the settings destination unwritable: replace the file with a
    // directory. fopen(path, "wb") fails on a directory on every platform —
    // including when the test runs as root, where chmod would not.
    std::error_code ec;
    std::filesystem::remove(settingsFile, ec);
    std::filesystem::create_directory(settingsFile, ec);
    ASSERT(std::filesystem::is_directory(settingsFile));

    // 3. Change a persistent parameter (SAVE_IMMEDIATELY saves on the
    // setter) and attempt an explicit save. Both writes must fail.
    dasher_set_speed_percent(ctx, 300);
    dasher_save_settings(ctx);

    // The failure is observable: the message callback fired exactly once
    // for this failure episode (the SAVE_IMMEDIATELY write and the explicit
    // save are two attempts at the same pending changes — a storm of
    // duplicate reports would be a regression), and it is non-modal: a
    // modal message (DASHER_MESSAGE_WARNING) pauses text entry.
    printf("  Save-failure messages received: %d (type %d)\n", failure_messages, failure_message_type);
    ASSERT_EQ(failure_messages, 1);
    ASSERT_EQ(failure_message_type, DASHER_MESSAGE_INFO);
    ASSERT(std::filesystem::is_directory(settingsFile));

    // 4. Restore write access and save again WITHOUT another parameter
    // change: the retained change must be persisted by the retry alone.
    std::filesystem::remove_all(settingsFile, ec);
    dasher_save_settings(ctx);
    ASSERT(std::filesystem::is_regular_file(settingsFile));
    // The successful retry reported nothing: success is silent, and the
    // episode's failure report was not repeated.
    ASSERT_EQ(failure_messages, 1);

    // 5. A SECOND failure episode must be reported again: the success above
    // resets the latch, so "latch once and stay silent forever" — or
    // "reset the latch but lose the modified flag" — would fail here.
    std::filesystem::remove(settingsFile, ec);
    std::filesystem::create_directory(settingsFile, ec);
    dasher_set_speed_percent(ctx, 350);
    dasher_save_settings(ctx);
    ASSERT_EQ(failure_messages, 2);
    std::filesystem::remove_all(settingsFile, ec);
    dasher_save_settings(ctx);
    ASSERT(std::filesystem::is_regular_file(settingsFile));

    dasher_destroy(ctx);
    ctx.ctx = nullptr; // destroyed explicitly above; skip the ScopedContext dtor

    // 6. The retries persisted the final value.
    std::unique_ptr<dasher_ctx, decltype(&dasher_destroy)> reloaded(
        dasher_create(TEST_DATA_DIR, ctx.dir.c_str(), nullptr), &dasher_destroy);
    ASSERT(reloaded != nullptr);
    ASSERT_EQ(dasher_get_speed_percent(reloaded.get()), 350);
}
