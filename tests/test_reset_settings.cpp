// dasher_reset_settings() tests: verifies that resetting restores every
// parameter type (bool / long / string) to its built-in default value and
// fires through the normal setter path so a live engine reflects the change.
#include "test_common.h"

TEST(reset_settings_null_ctx_is_safe) {
    // Must not crash on a null context.
    dasher_reset_settings(nullptr);
    ASSERT(true);
}

TEST(reset_settings_restores_bool_parameter) {
    ScopedContext ctx(800, 600);
    ASSERT(ctx != nullptr);

    const int key = dasher_find_parameter_key("BP_COPY_ALL_ON_STOP");
    ASSERT(key >= 0);

    const int default_value = dasher_get_bool_parameter(ctx, key);
    // Flip away from the default, then reset.
    dasher_set_bool_parameter(ctx, key, default_value ? 0 : 1);
    ASSERT_NEQ(dasher_get_bool_parameter(ctx, key), default_value);

    dasher_reset_settings(ctx);
    ASSERT_EQ(dasher_get_bool_parameter(ctx, key), default_value);
}

TEST(reset_settings_restores_long_parameter) {
    ScopedContext ctx(800, 600);
    ASSERT(ctx != nullptr);

    const int key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(key >= 0);

    const long default_value = dasher_get_long_parameter(ctx, key);
    // Move well away from the default, then reset.
    dasher_set_long_parameter(ctx, key, default_value + 1000);
    ASSERT_NEQ(dasher_get_long_parameter(ctx, key), default_value);

    dasher_reset_settings(ctx);
    ASSERT_EQ(dasher_get_long_parameter(ctx, key), default_value);
}

TEST(reset_settings_restores_string_parameter) {
    ScopedContext ctx(800, 600);
    ASSERT(ctx != nullptr);

    const int key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(key >= 0);

    // Snapshot the default alphabet, then switch to a different one that is
    // actually bundled (picked from the available values list, so this is
    // robust to data changes and never relies on a hardcoded name).
    const char* default_ptr = dasher_get_string_parameter(ctx, key);
    ASSERT(default_ptr != nullptr);
    const std::string default_value(default_ptr);

    const char* values_buf[32] = {nullptr};
    const int count = dasher_get_parameter_string_values(ctx, key, values_buf, 32);
    ASSERT(count > 0);

    std::string other_value;
    for (int i = 0; i < count; ++i) {
        if (values_buf[i] == nullptr) continue;
        const std::string candidate(values_buf[i]);
        if (candidate != default_value) {
            other_value = candidate;
            break;
        }
    }
    ASSERT(!other_value.empty());

    dasher_set_string_parameter(ctx, key, other_value.c_str());
    ASSERT_STR_EQ(dasher_get_string_parameter(ctx, key), other_value);

    dasher_reset_settings(ctx);
    ASSERT_STR_EQ(dasher_get_string_parameter(ctx, key), default_value);
}

TEST(reset_settings_is_idempotent) {
    ScopedContext ctx(800, 600);
    ASSERT(ctx != nullptr);

    const int key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(key >= 0);
    const long default_value = dasher_get_long_parameter(ctx, key);

    dasher_reset_settings(ctx);
    dasher_reset_settings(ctx);
    ASSERT_EQ(dasher_get_long_parameter(ctx, key), default_value);
}
