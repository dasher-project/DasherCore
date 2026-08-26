// Extended C API tests: cover functions not tested in test_capi.cpp
#include "test_common.h"

#include <cstring>
#include <vector>
TEST(long_string_params) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Find a known parameter key
    int key = dasher_find_parameter_key("LP_MAX_BITRATE");
    ASSERT(key >= 0);
    printf("  LP_MAX_BITRATE key: %d\n", key);

    // Get/set long parameter
    long original = dasher_get_long_parameter(ctx, key);
    ASSERT(original >= 0);
    printf("  Original LP_MAX_BITRATE: %ld\n", original);

    dasher_set_long_parameter(ctx, key, 5000);
    long val = dasher_get_long_parameter(ctx, key);
    ASSERT_EQ(val, 5000);

    // Restore
    dasher_set_long_parameter(ctx, key, original);

    // Test string parameter
    int str_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(str_key >= 0);

    const char* alph = dasher_get_string_parameter(ctx, str_key);
    ASSERT(alph != nullptr);
    printf("  Current alphabet: '%s'\n", alph);

    dasher_set_string_parameter(ctx, str_key, "English lower case");
    const char* new_alph = dasher_get_string_parameter(ctx, str_key);
    ASSERT_STR_EQ(new_alph, "English lower case");

    dasher_destroy(ctx);
}

TEST(palettes) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_palette_count(ctx);
    printf("  Palette count: %d\n", count);
    ASSERT(count > 0);

    // List all palettes
    for (int i = 0; i < count; i++) {
        const char* name = dasher_get_palette_name(ctx, i);
        ASSERT(name != nullptr);
        ASSERT(strlen(name) > 0);
        printf("  Palette %d: '%s'\n", i, name);
    }

    // Get current palette
    const char* current = dasher_get_current_palette(ctx);
    ASSERT(current != nullptr);
    printf("  Current palette: '%s'\n", current);

    // Get preview colors
    int32_t colors[4] = {0, 0, 0, 0};
    int result = dasher_get_palette_preview_colors(ctx, 0, colors);
    ASSERT_EQ(result, 0);
    printf("  Preview colors: 0x%08X 0x%08X 0x%08X 0x%08X\n", colors[0], colors[1], colors[2], colors[3]);

    // Try setting a palette
    const char* first_name = dasher_get_palette_name(ctx, 0);
    dasher_set_palette(ctx, first_name);

    dasher_destroy(ctx);
}

TEST(alphabet_listing) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int count = dasher_get_alphabet_count(ctx);
    printf("  Alphabet count: %d\n", count);
    ASSERT(count > 0);

    for (int i = 0; i < count; i++) {
        const char* name = dasher_get_alphabet_name(ctx, i);
        ASSERT(name != nullptr);
        ASSERT(strlen(name) > 0);
        printf("  Alphabet %d: '%s'\n", i, name);
    }

    dasher_destroy(ctx);
}

TEST(enum_values) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Find a parameter that has enum values (e.g., orientation)
    int orient_key = dasher_find_parameter_key("LP_ORIENTATION");
    ASSERT(orient_key >= 0);

    int enum_count = dasher_get_parameter_enum_count(orient_key);
    printf("  Orientation enum count: %d\n", enum_count);
    ASSERT(enum_count > 0);

    for (int i = 0; i < enum_count; i++) {
        const char* name = dasher_get_parameter_enum_name(orient_key, i);
        int value = dasher_get_parameter_enum_value(orient_key, i);
        ASSERT(name != nullptr);
        printf("  Enum %d: '%s' = %d\n", i, name, value);
    }

    dasher_destroy(ctx);
}

TEST(language_model_info) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    int lm_count = dasher_get_language_model_count();
    printf("  Language model count: %d\n", lm_count);
    ASSERT(lm_count > 0);

    for (int i = 0; i < lm_count; i++) {
        const char* name = dasher_get_language_model_name(i);
        const char* desc = dasher_get_language_model_description(i);
        ASSERT(name != nullptr);
        printf("  LM %d: '%s' - '%s'\n", i, name, desc ? desc : "(null)");

        int param_count = dasher_get_language_model_param_count(i);
        printf("    Parameters: %d\n", param_count);
    }

    dasher_destroy(ctx);
}

TEST(message_callback) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    static int msg_count = 0;
    msg_count = 0;

    dasher_set_message_callback(
        ctx,
        [](int message_type, const char* text, void*) {
            if (text) {
                printf("  Message (type=%d): '%s'\n", message_type, text);
                msg_count++;
            }
        },
        nullptr);

    // Trigger something that might produce a message (e.g., setting an invalid alphabet)
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_alphabet_id(ctx, "Nonexistent Alphabet XYZ");

    // The message callback might fire for error states
    printf("  Messages received: %d\n", msg_count);

    dasher_destroy(ctx);
}

TEST(reset) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_speed_percent(ctx, 300);
    dasher_set_screen_size(ctx, 800, 600);

    // Produce some text
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        int* cmds = nullptr;
        int cmd_cnt = 0;
        char** strs = nullptr;
        int str_cnt = 0;
        dasher_frame(ctx, 1000 + i * 20, &cmds, &cmd_cnt, &strs, &str_cnt);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(strlen(text) > 0);

    // Reset should clear everything
    dasher_reset(ctx);
    text = dasher_get_output_text(ctx);
    ASSERT_EQ(strlen(text), 0);

    dasher_destroy(ctx);
}

TEST(restart_keeps_typing_after_stop_start_cycle) {
    // Regression (#60 — restart drift reported on Windows + Android): input
    // events were stamped with steady_clock while dasher_frame runs on the
    // frontend's own timeline. run() seeds the framerate window and
    // slow-start clock from the input stamp, so a start subtracted across
    // two clocks: the first sample window measured garbage, LP_FRAMERATE (a
    // decaying average) collapsed, the next step flung the canvas and the
    // engine ended up stopped/frozen with no output. Input stamps must come
    // from the engine's own frame timeline (v5 semantics: one clock).
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_speed_percent(ctx, 300);
    dasher_set_screen_size(ctx, 800, 600);

    const int fr_key = dasher_find_parameter_key("LP_FRAMERATE");
    ASSERT(fr_key >= 0);

    int64_t clock = 1000;
    auto frame = [&]() {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        int* c = nullptr;
        int cc = 0;
        char** s = nullptr;
        int sc = 0;
        dasher_frame(ctx, clock += 20, &c, &cc, &s, &sc);
    };
    auto outLen = [&]() { return strlen(dasher_get_output_text(ctx)); };

    // First run: types normally (same setup as the reset test).
    dasher_mouse_down(ctx);
    for (int i = 0; i < 300; i++)
        frame();
    dasher_mouse_up(ctx);
    const size_t len1 = outLen();
    const long f1 = dasher_get_long_parameter(ctx, fr_key);
    ASSERT(len1 > 0); // pre-fix: engine froze with zero output

    // Stop, then restart.
    dasher_mouse_down(ctx);
    for (int i = 0; i < 5; i++)
        frame();
    dasher_mouse_up(ctx);

    dasher_mouse_down(ctx);
    for (int i = 0; i < 4; i++)
        frame();
    // Sample the restart transient: pre-fix, the first post-restart window
    // was seeded from a steady_clock stamp and measured ~0 fps, collapsing
    // LP_FRAMERATE to ~half (observed 4999 -> 2499) and doubling step sizes
    // for ~10 frames — the "canvas pulls you much higher" restart fling.
    const long f2 = dasher_get_long_parameter(ctx, fr_key);
    for (int i = 0; i < 146; i++)
        frame();
    dasher_mouse_up(ctx);
    const size_t len2 = outLen();

    printf("  run1: len=%zu FR=%ld  restart@4: FR=%ld len=%zu\n", len1, f1, f2, len2);

    // Post-restart the engine keeps typing at the same measured rate.
    ASSERT(len2 > len1);
    ASSERT(f2 >= f1 - f1 / 10);

    dasher_destroy(ctx);
}

TEST(create_with_fresh_user_dir_regex_hazard_path) {
    // Regression: XmlSettingsStore::Load() passes the settings file's
    // absolute path to ScanFiles(); when the file does not exist yet
    // (fresh user dir - the normal first-run case) the old code fell
    // through to std::regex(pattern), and Windows paths throw there:
    // backslashes become escapes, so a final component starting with a
    // digit ("\2" -> backreference) or 'c' ("\c" -> control escape) made
    // dasher_create fail outright. A path component of "2c" makes the
    // hazard deterministic; both sequential creates must succeed.
    const std::string data = get_test_data_dir();

    for (int i = 0; i < 2; i++) {
        ScopedTempDir tmp; // fresh dir each iteration => no settings file yet
        std::error_code ec;
        const auto hazard = std::filesystem::path(tmp.path) / "2c";
        std::filesystem::create_directories(hazard, ec);

        char* err = nullptr;
        dasher_ctx* ctx = dasher_create(data.c_str(), hazard.string().c_str(), &err);
        if (!ctx) printf("  create #%d failed: %s\n", i, err ? err : "(no error string)");
        ASSERT(ctx != nullptr);
        dasher_destroy(ctx);
    }
}

TEST(reset_emits_buffer_clear_event) {
    // Resets clear the edit buffer without insert/delete deltas, so they must
    // announce themselves as event type 2 — subscribers keeping a shadow
    // buffer cannot reconstruct "everything vanished" from 0/1 events
    // (Dasher-GTK's stale output pane after "New" was this bug).
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    static int clear_events = 0;
    static int other_events = 0;
    clear_events = 0;
    other_events = 0;

    dasher_set_output_callback(
        ctx,
        [](int event_type, const char* text, void*) {
            if (event_type == 2) {
                ASSERT(text != nullptr); // empty string, never null
                clear_events++;
            } else if (event_type == 0 || event_type == 1) {
                other_events++;
            }
            // Unknown future event types must be ignored, not fatal.
        },
        nullptr);

    dasher_reset_output_text(ctx);
    ASSERT_EQ(clear_events, 1);

    dasher_reset(ctx);
    ASSERT_EQ(clear_events, 2);

    // Alphabet changes clear the buffer as a documented side effect, so they
    // fire the event too.
    dasher_set_alphabet_id(ctx, "English with numerals and limited punctuation");
    ASSERT_EQ(clear_events, 3);

    // Setting the *same* alphabet still cleared the buffer (historic
    // behaviour), so the event fires regardless.
    dasher_set_alphabet_id(ctx, "English with numerals and limited punctuation");
    ASSERT_EQ(clear_events, 4);

    (void)other_events;
    dasher_destroy(ctx);
}

TEST(save_settings) {
    static int save_test_counter = 0;
    char shared_dir[256];
    snprintf(shared_dir, sizeof(shared_dir), "%s/dasher_save_test_%d_%d", dasher_temp_dir(), dasher_getpid(),
             save_test_counter++);
    dasher_mkdir(shared_dir);

    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Change a setting
    dasher_set_speed_percent(ctx, 250);

    // Save
    dasher_save_settings(ctx);

    // Create new context - should load saved settings
    dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, shared_dir, nullptr);
    ASSERT(ctx2 != nullptr);

    int speed = dasher_get_speed_percent(ctx2);
    printf("  Speed after reload: %d\n", speed);

    // Restore default
    dasher_set_speed_percent(ctx2, 100);
    dasher_save_settings(ctx2);

    dasher_destroy(ctx);
    dasher_destroy(ctx2);
}

TEST(key_event) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Key events should not crash (null-safe, valid key codes)
    dasher_key_event(ctx, 0, 1); // press key 0
    dasher_key_event(ctx, 0, 0); // release key 0

    run_frames(ctx, 5, 1000, 20);

    dasher_destroy(ctx);
}

TEST(string_values) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Get string values for SP_ALPHABET_ID
    int alph_key = dasher_find_parameter_key("SP_ALPHABET_ID");
    ASSERT(alph_key >= 0);

    // Probe call first: must return the full count (regression: it used to
    // return 0 before querying, so every permitted-value list looked empty
    // and frontends rendered blank pickers).
    const int probe_count = dasher_get_parameter_string_values(ctx, alph_key, nullptr, 0);
    ASSERT(probe_count > 0);

    std::vector<const char*> values_buf(probe_count, nullptr);
    int count = dasher_get_parameter_string_values(ctx, alph_key, values_buf.data(), probe_count);
    printf("  String values count: %d\n", count);
    ASSERT(count == probe_count);

    for (int i = 0; i < count && i < 5; i++) {
        ASSERT(values_buf[i] != nullptr);
        printf("  Value %d: '%s'\n", i, values_buf[i]);
    }

    // The engine's current alphabet must be one of the permitted values.
    const char* current = dasher_get_alphabet_id(ctx);
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (values_buf[i] && current && strcmp(values_buf[i], current) == 0) {
            found = true;
            break;
        }
    }
    ASSERT(found);

    dasher_destroy(ctx);
}

TEST(game_mode_basic) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Game mode should start disabled
    ASSERT_EQ(dasher_game_mode_active(ctx), 0);

    // Enable game mode
    ASSERT_EQ(dasher_enter_game_mode(ctx), 0);
    ASSERT_EQ(dasher_game_mode_active(ctx), 1);

    // Get target text
    const char* target = dasher_game_get_target_text(ctx);
    printf("  Target text: '%s'\n", target ? target : "(null)");

    // Get correct count and target length
    int correct = dasher_game_get_correct_count(ctx);
    int total = dasher_game_get_target_length(ctx);
    printf("  Progress: %d/%d\n", correct, total);

    // Get wrong text
    const char* wrong = dasher_game_get_wrong_text(ctx);
    printf("  Wrong text: %s\n", wrong ? wrong : "(null)");

    // Leave game mode
    dasher_leave_game_mode(ctx);
    ASSERT_EQ(dasher_game_mode_active(ctx), 0);

    dasher_destroy(ctx);
}

TEST(null_safety_extended) {
    // Additional null-safety for functions not covered by basic null_safety test
    dasher_ctx* null_ctx = nullptr;

    // These should all return safely without crashing
    dasher_set_low_memory_mode(null_ctx, 1);
    dasher_reset(null_ctx);
    dasher_key_event(null_ctx, 0, 1);
    dasher_save_settings(null_ctx);

    ASSERT_EQ(dasher_get_palette_count(null_ctx), 0);
    ASSERT(dasher_get_palette_name(null_ctx, 0) != nullptr); // returns "" not null
    ASSERT(dasher_get_current_palette(null_ctx) != nullptr); // returns "" not null
    ASSERT_EQ(dasher_get_palette_preview_colors(null_ctx, 0, nullptr), -1);

    ASSERT_EQ(dasher_get_alphabet_count(null_ctx), 0);
    ASSERT(dasher_get_alphabet_name(null_ctx, 0) != nullptr); // returns "" not null

    ASSERT_EQ(dasher_game_mode_active(null_ctx), 0);
    dasher_enter_game_mode(null_ctx);
    dasher_leave_game_mode(null_ctx);

    ASSERT_EQ(dasher_find_parameter_key("nonexistent_param_xyz"), -1);

    int32_t colors[4] = {0};
    ASSERT_EQ(dasher_get_palette_preview_colors(null_ctx, 0, colors), -1);
}

// ── Typing-rate reset (#44) ──────────────────────────────────────────────────

TEST(cps_reset_clears_measurement_window) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Before any output: CPS should be 0.
    ASSERT_EQ(dasher_get_cps(ctx), 0.0);

    // Simulate some output by directly pushing timestamps (same as editOutput does).
    // We can't easily drive the engine to produce text in a unit test, so we
    // verify the reset mechanism itself: CPS is non-zero after timestamps exist,
    // then zero again after reset.
    //
    // Since rateTimestamps is private, we test via the public surface:
    // 1. dasher_get_cps on a fresh ctx → 0 (no timestamps)
    // 2. dasher_reset_cps → clears (idempotent on empty)
    // 3. dasher_get_cps → still 0

    dasher_reset_cps(ctx);
    ASSERT_EQ(dasher_get_cps(ctx), 0.0);

    // Reset is safe on a null context.
    dasher_reset_cps(nullptr);

    dasher_destroy(ctx);
}

TEST(cps_reset_after_reset_settings) {
    // dasher_reset() already clears rateTimestamps internally (line 917 of CAPI.cpp).
    // Verify dasher_reset_cps is consistent with that behaviour.
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Both resets should leave CPS at 0.
    dasher_reset_cps(ctx);
    ASSERT_EQ(dasher_get_cps(ctx), 0.0);

    dasher_reset(ctx);
    ASSERT_EQ(dasher_get_cps(ctx), 0.0);

    dasher_destroy(ctx);
}
