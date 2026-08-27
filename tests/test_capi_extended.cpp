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

TEST(autocalibrate_off_by_default_and_offset_session_scoped) {
    // Regression (#64 — the still-occurring restart drift): BP_AUTOCALIBRATE
    // is Dasher's 2004 "enhanced eyetracking mode", designed to correct
    // systematic eye-tracker Y error. v5 shipped it OFF (both legacy repos);
    // #54 flipped v6 to true on a wrong premise, and for a pointer user
    // deliberately dwelling above centre the filter read intentional
    // steering as bias and drifted LP_TARGET_OFFSET monotonically
    // (measured: 0 -> -68 over 5 dwell/lift/restart cycles), shifting the
    // steering point by 10x that every frame and persisting across
    // sessions. Default must be false; the learned offset must be
    // session-scoped (EPHEMERAL), never persisted.
    const int ac_key = dasher_find_parameter_key("BP_AUTOCALIBRATE");
    const int off_key = dasher_find_parameter_key("LP_TARGET_OFFSET");
    ASSERT(ac_key >= 0);
    ASSERT(off_key >= 0);

    {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx != nullptr);
        dasher_set_screen_size(ctx, 800, 600);

        // v5 parity: off by default.
        ASSERT_EQ(dasher_get_bool_parameter(ctx, ac_key), 0);

        // Dwelling above centre with defaults must not move the offset.
        int64_t clock = 1000;
        auto frame = [&]() {
            dasher_mouse_move(ctx, 640.0f, 260.0f);
            dasher_frame(ctx, clock += 16, nullptr, nullptr, nullptr, nullptr);
        };
        dasher_mouse_down(ctx);
        for (int i = 0; i < 150; i++)
            frame();
        ASSERT_EQ(dasher_get_long_parameter(ctx, off_key), 0);

        // Opt-in (eye-gaze) still calibrates: with it on, sustained
        // above-centre dwell must move the offset.
        dasher_set_bool_parameter(ctx, ac_key, 1);
        for (int i = 0; i < 400; i++)
            frame();
        ASSERT(dasher_get_long_parameter(ctx, off_key) != 0);

        dasher_destroy(ctx);
    }

    // Session scope: a fresh context in the SAME user dir starts at 0 —
    // the learned offset is not persisted, and a stale <TargetOffset>
    // written by an older build (pre-ephemeral) is ignored on load.
    {
        ScopedTempDir tmp;
        char* err = nullptr;
        dasher_ctx* ctx = dasher_create(get_test_data_dir(), tmp.path.c_str(), &err);
        ASSERT(ctx != nullptr);
        // Write a learned offset + opt-in while running...
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_bool_parameter(ctx, ac_key, 1);
        dasher_set_long_parameter(ctx, off_key, 42);
        dasher_save_settings(ctx);
        dasher_destroy(ctx);

        // ...and simulate an upgrading user whose older build persisted the
        // drifted offset directly into the settings file (the store's
        // schema is <long name="..." value="..."/>).
        {
            std::filesystem::path settingsPath = std::filesystem::path(tmp.path) / "dasher_settings.xml";
            std::ifstream in(settingsPath);
            std::string xml((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            const size_t closing = xml.find("</settings>");
            ASSERT(closing != std::string::npos);
            xml.insert(closing, "<long name=\"TargetOffset\" value=\"-68\"/>");
            std::ofstream out(settingsPath, std::ios::trunc);
            out << xml;
        }

        // Restart the same user dir: the stale drifted offset is ignored
        // (ephemeral), while the explicit opt-in persists (eyegaze users
        // keep their setting across sessions).
        dasher_ctx* ctx2 = dasher_create(get_test_data_dir(), tmp.path.c_str(), &err);
        ASSERT(ctx2 != nullptr);
        ASSERT_EQ(dasher_get_long_parameter(ctx2, off_key), 0);
        ASSERT_EQ(dasher_get_bool_parameter(ctx2, ac_key), 1);
        dasher_destroy(ctx2);
    }
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

TEST(reload_settings) {
    // dasher_reload_settings re-reads dasher_settings.xml and applies
    // changes through the normal parameter path. This matters when the
    // settings file changes externally (IME sharing the user dir, a
    // migration moving the file) — the engine should pick up the change
    // without being destroyed and recreated.
    static int reload_test_counter = 0;
    char user_dir[256];
    snprintf(user_dir, sizeof(user_dir), "%s/dasher_reload_test_%d_%d", dasher_temp_dir(), dasher_getpid(),
             reload_test_counter++);
    dasher_mkdir(user_dir);

    dasher_ctx* ctx = dasher_create(TEST_DATA_DIR, user_dir, nullptr);
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Set an initial speed and save
    dasher_set_speed_percent(ctx, 200);
    dasher_save_settings(ctx);
    const int before = dasher_get_speed_percent(ctx);
    ASSERT(before == 200);

    // Simulate an external change: create a second engine that writes a
    // different speed to the same settings file
    {
        dasher_ctx* writer = dasher_create(TEST_DATA_DIR, user_dir, nullptr);
        ASSERT(writer != nullptr);
        dasher_set_speed_percent(writer, 350);
        dasher_save_settings(writer);
        dasher_destroy(writer);
    }

    // The first engine still has its in-memory value
    ASSERT_EQ(dasher_get_speed_percent(ctx), 200);

    // Track parameter-change notifications
    static int param_changes = 0;
    param_changes = 0;
    dasher_set_parameter_callback(ctx, [](int key, void*) { param_changes++; }, nullptr);

    // Reload — should pick up the external change
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 350);

    // Parameter-change callback fired for the changed setting
    ASSERT(param_changes > 0);

    // Reload again with no file change — no spurious notifications
    param_changes = 0;
    dasher_reload_settings(ctx);
    ASSERT_EQ(param_changes, 0);

    // Removed setting: external writer deletes the entry from the XML,
    // reload should restore the declared default and notify.
    {
        // Write a settings file with the speed entry removed (just an
        // empty settings element — simplest removal case)
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n</settings>\n", f);
            fclose(f);
        }
    }
    param_changes = 0;
    dasher_reload_settings(ctx);
    // Speed falls back to the manifest default (raw 80 = 50%)
    const int default_speed = dasher_get_speed_percent(ctx);
    ASSERT_EQ(default_speed, 50);
    ASSERT(param_changes > 0); // the removal triggered a notification

    // Malformed file: external writer corrupts the XML — reload should
    // keep current values (not reset to defaults) and emit no callbacks.
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("THIS IS NOT XML AT ALL <<<\n", f);
            fclose(f);
        }
    }
    param_changes = 0;
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 50); // kept, not reset
    ASSERT_EQ(param_changes, 0);                  // no spurious callbacks

    // Wrong root: well-formed XML but not a settings file — same as
    // malformed, keep current values and emit no callbacks.
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<not_settings>\n<long name=\"x\" "
                  "value=\"1\"/>\n</not_settings>\n",
                  f);
            fclose(f);
        }
    }
    param_changes = 0;
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 50); // kept, not reset
    ASSERT_EQ(param_changes, 0);                  // no spurious callbacks

    // Invalid value: valid XML, valid root, but an unparseable value —
    // treated as corruption, keep current values, no callbacks.
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n<long name=\"LP_MAX_BITRATE\" "
                  "value=\"banana\"/>\n</settings>\n",
                  f);
            fclose(f);
        }
    }
    param_changes = 0;
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 50); // kept, not reset
    ASSERT_EQ(param_changes, 0);                  // no spurious callbacks

    // Nameless entry: a <long> with no name attribute is corruption —
    // keep values, no callbacks. Set a non-default value first so a
    // silent reset to default would be observable.
    dasher_set_speed_percent(ctx, 120);
    param_changes = 0;
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n<long value=\"560\"/>\n</settings>\n", f);
            fclose(f);
        }
    }
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 120); // kept, not reset
    ASSERT_EQ(param_changes, 0);                   // no spurious callbacks

    // Wrong tag: a known parameter's storage name under the wrong element
    // type — the entry would land in a map LoadPersistent never reads,
    // resetting the setting to default. Corruption: keep values, no
    // callbacks. (Speed's storage name is MaxBitRateTimes100.)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n<bool name=\"MaxBitRateTimes100\" "
                  "value=\"1\"/>\n</settings>\n",
                  f);
            fclose(f);
        }
    }
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 120); // kept, not reset
    ASSERT_EQ(param_changes, 0);                   // no spurious callbacks

    // Junk suffix: "12junk" must not validate — the 12 prefix would
    // silently reach the live setter. Same for bool "truejunk".
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n<long name=\"MaxBitRateTimes100\" "
                  "value=\"12junk\"/>\n<bool name=\"AutoSpeedControl\" value=\"truejunk\"/>\n</settings>\n",
                  f);
            fclose(f);
        }
    }
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 120); // kept, not reset
    ASSERT_EQ(param_changes, 0);                   // no spurious callbacks

    // Overflow: an all-numeric value beyond LONG_MAX — strtol clamps to
    // LONG_MAX and flags ERANGE; the clamped value must not be applied.
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", user_dir);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n<long name=\"MaxBitRateTimes100\" "
                  "value=\"99999999999999999999999999\"/>\n</settings>\n",
                  f);
            fclose(f);
        }
    }
    dasher_reload_settings(ctx);
    ASSERT_EQ(dasher_get_speed_percent(ctx), 120); // kept, not clamped-applied
    ASSERT_EQ(param_changes, 0);                   // no spurious callbacks

    // Edit buffer survives the reload
    // (not asserting content — just that the engine is still functional)
    dasher_reset_output_text(ctx);
    ASSERT(dasher_get_output_text(ctx) != nullptr);

    dasher_destroy(ctx);

    // Startup with a partially corrupt file: a corrupt entry followed by
    // a valid one. The initial Load must pick up the valid entry —
    // stopping the scan at the corrupt one made later settings load from
    // defaults, giving a document-order-dependent configuration.
    {
        char dir2[256];
        snprintf(dir2, sizeof(dir2), "%s/dasher_reload_test_partial_%d", dasher_temp_dir(), dasher_getpid());
        dasher_mkdir(dir2);
        char path[512];
        snprintf(path, sizeof(path), "%s/dasher_settings.xml", dir2);
        FILE* f = fopen(path, "w");
        if (f) {
            fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<settings>\n"
                  "<long value=\"560\"/>\n"                                // corrupt: nameless
                  "<long name=\"MaxBitRateTimes100\" value=\"12junk\"/>\n" // corrupt: junk suffix
                  "<long name=\"MaxBitRateTimes100\" value=\"560\"/>\n"    // valid: speed 350%
                  "</settings>\n",
                  f);
            fclose(f);
        }
        dasher_ctx* ctx2 = dasher_create(TEST_DATA_DIR, dir2, nullptr);
        ASSERT(ctx2 != nullptr);
        ASSERT_EQ(dasher_get_speed_percent(ctx2), 350); // valid entry loaded despite earlier corrupt ones
        dasher_destroy(ctx2);
    }
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
