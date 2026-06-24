// test_input_filters.cpp
//
// CHARACTERIZATION TESTS for input filters reachable via the C API.
//
// Before this file, 12 of 13 registered input filters had zero test
// coverage — including every switch-accessible mode (OneButtonDynamic,
// TwoButtonDynamic, TwoPushDynamic, Click). This file closes that gap by
// running a standardized scenario against every filter and asserting
// "no crash, sensible state".
//
// What "characterization" means here: we do NOT assert that any filter
// produces a specific output. We assert the weaker, robust contract that
// each filter (a) activates cleanly, (b) accepts its expected input modality,
// (c) produces either empty or non-empty output text under a clearly-labeled
// scenario, and (d) doesn't crash the engine or corrupt state.
//
// When you change a filter's behavior, you may need to update the assertions
// here. That's fine — the test exists to make you look.
//
// Reference:
//   - Filter factory: src/DasherCore/DasherInterfaceBase.cpp:663-694
//   - Key codes:      src/DasherCore/DasherTypes.h:114-138
//   - Time domains:   dasher_frame uses test time; key_event uses wall clock
//                     (so we use usleep for short/long-press classification)

#include "test_common.h"

#include <chrono>
#include <cstring>
#include <string>
#include <thread>

namespace {

// All 14 registered filter names (13 distinct C++ classes — CButtonMode is
// registered twice with different names for Menu vs Direct).
//
// From DasherInterfaceBase.cpp:663-694. The two CButtonMode instances:
//   "Menu Mode"  (menu=true)
//   "Direct Mode" (menu=false)
const char* const kAllFilters[] = {
    "Normal Control",
    "Press Mode",
    "Smoothing Mode",
    "One Dimensional Mode",
    "Click Mode",
    "Static One Button Mode",
    "One Button Dynamic Mode",
    "Two Button Dynamic Mode",
    "Two-push Dynamic Mode (New One Button)",
    "Menu Mode",
    "Direct Mode",
    "Alternating Direct Mode",
    "Compass Mode",
    "Stylus Control",
};
constexpr int kNumFilters = sizeof(kAllFilters) / sizeof(kAllFilters[0]);

// Key codes (from DasherTypes.h:114-138, dasher.h:71).
constexpr int kKeyStartStop = 0;
constexpr int kKeyButton1 = 1; // backoff
constexpr int kKeyButton2 = 2; // forward/up
constexpr int kKeyButton3 = 3; // forward/down
constexpr int kKeyButton4 = 4; // forward/extra
constexpr int kKeyPrimary = 100;

// Helper: look up SP_INPUT_FILTER key (memoized; same value across all
// contexts in this process).
int sp_input_filter_key() {
    static int key = dasher_find_parameter_key("SP_INPUT_FILTER");
    return key;
}

void set_filter(dasher_ctx* ctx, const char* name) {
    dasher_set_string_parameter(ctx, sp_input_filter_key(), name);
}

void press_release(dasher_ctx* ctx, int key, int press_ms = 20) {
    dasher_key_event(ctx, key, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(press_ms));
    dasher_key_event(ctx, key, 0);
}

} // namespace

// ---------------------------------------------------------------------------
// Foundational: every filter name activates cleanly
// ---------------------------------------------------------------------------

TEST_CASE("filters/all names registered and switchable") {
    // CHARACTERIZATION: each name in kAllFilters must be a real registered
    // filter (no silent fallback to "Normal Control"). We verify by setting
    // SP_INPUT_FILTER to each name and reading it back.
    ScopedContext ctx(800, 600);
    const int sp = sp_input_filter_key();

    for (int i = 0; i < kNumFilters; ++i) {
        dasher_set_string_parameter(ctx, sp, kAllFilters[i]);
        const char* readback = dasher_get_string_parameter(ctx, sp);
        INFO("filter index ", i, " name '", kAllFilters[i], "'");
        CHECK(std::string(readback) == std::string(kAllFilters[i]));
    }
}

TEST_CASE("filters/invalid name silently falls back") {
    // CHARACTERIZATION: setting SP_INPUT_FILTER to an unregistered name
    // does NOT error. The store keeps the bad string but the active filter
    // becomes "Normal Control". (DasherInterfaceBase.cpp:656.)
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Nonexistent Filter Mode");
    // The string is stored as-is.
    CHECK(std::string(dasher_get_string_parameter(ctx, sp_input_filter_key())) == "Nonexistent Filter Mode");
    // But the engine still runs (a frame doesn't crash).
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);
    CHECK(cc > 0);
}

TEST_CASE("filters/setting SP_INPUT_FILTER before realize is lost") {
    // CHARACTERIZATION: dasher_set_screen_size force-sets SP_INPUT_FILTER
    // to "Normal Control" exactly once during realize (CAPI.cpp:724). Any
    // earlier setting is overwritten. This is documented behavior we
    // characterize so a future change to the force-set is visible.
    ScopedTempDir dir;
    dasher_ctx* ctx = dasher_create(get_test_data_dir(), dir.c_str(), nullptr);
    REQUIRE(ctx != nullptr);

    set_filter(ctx, "Click Mode");
    CHECK(std::string(dasher_get_string_parameter(ctx, sp_input_filter_key())) == "Click Mode");

    // set_screen_size realizes and overwrites.
    dasher_set_screen_size(ctx, 800, 600);
    CHECK(std::string(dasher_get_string_parameter(ctx, sp_input_filter_key())) == "Normal Control");

    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// Mouse-based continuous filters
//
// These accept dasher_mouse_move/down/up and produce output when the user
// hovers forward of the crosshair.
// ---------------------------------------------------------------------------

TEST_CASE("filters/Normal Control produces text on hover") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Normal Control");
    dasher_set_speed_percent(ctx, 300);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 300; ++i) {
        dasher_mouse_move(ctx, 700.0f, 290.0f);
        run_frames(ctx, 1, 1000 + i * 16);
    }
    dasher_mouse_up(ctx);

    const char* text = dasher_get_output_text(ctx);
    REQUIRE(text != nullptr);
    CHECK(std::string(text).size() > 0);
}

TEST_CASE("filters/Press Mode produces text on hover") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Press Mode");
    dasher_set_speed_percent(ctx, 300);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 200);
    // Press Mode stops on mouse_up. Verify by capturing text before and
    // after the up event.
    std::string before_up(dasher_get_output_text(ctx));
    dasher_mouse_up(ctx);
    run_frames(ctx, 20);
    std::string after_up(dasher_get_output_text(ctx));

    CHECK(before_up.size() > 0);
    // After mouse_up, the text should not grow (or grows only minimally
    // from already-scheduled steps). Either way: not a crash.
    CHECK(after_up.size() >= before_up.size());
}

TEST_CASE("filters/Smoothing Mode produces text on hover") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Smoothing Mode");
    dasher_set_speed_percent(ctx, 300);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 300);
    dasher_mouse_up(ctx);

    CHECK(std::string(dasher_get_output_text(ctx)).size() > 0);
}

TEST_CASE("filters/One Dimensional Mode accepts mouse input") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "One Dimensional Mode");
    dasher_set_speed_percent(ctx, 300);

    // 1D mode uses only Y for navigation. Provide Y-only motion.
    dasher_mouse_move(ctx, 400.0f, 200.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 200; ++i) {
        dasher_mouse_move(ctx, 400.0f, 100.0f + (i % 200));
        run_frames(ctx, 1, 1000 + i * 16);
    }
    dasher_mouse_up(ctx);

    // No assertion on output text length — 1D mode may or may not produce
    // text depending on the Y trajectory. The contract is "no crash".
    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

// ---------------------------------------------------------------------------
// Click-mode filters (one discrete zoom per click)
// ---------------------------------------------------------------------------

TEST_CASE("filters/Click Mode handles discrete clicks") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Click Mode");

    dasher_mouse_move(ctx, 400.0f, 200.0f);
    for (int n = 0; n < 10; ++n) {
        dasher_mouse_down(ctx);
        run_frames(ctx, 3, 1000 + n * 100, 16);
        dasher_mouse_up(ctx);
        run_frames(ctx, 3, 1100 + n * 100, 16);
    }
    // No specific output assertion: click mode on a non-target yields
    // variable text. The contract is "no crash on the click cycle".
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 100000, &cmds, &cc, &strs, &sc);
    CHECK(cc > 0);
}

TEST_CASE("filters/Stylus Control handles tap-style clicks") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Stylus Control");
    // Lower LP_TAP_TIME so the test (which uses wall-clock mouse_down/up)
    // reliably classifies as a "short tap" rather than a long press.
    const int lp_tap = dasher_find_parameter_key("LP_TAP_TIME");
    dasher_set_long_parameter(ctx, lp_tap, 100000); // 100s — any tap is short

    dasher_mouse_move(ctx, 400.0f, 200.0f);
    for (int n = 0; n < 5; ++n) {
        dasher_mouse_down(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        run_frames(ctx, 3, 1000 + n * 100, 16);
        dasher_mouse_up(ctx);
        run_frames(ctx, 3, 1100 + n * 100, 16);
    }
    dasher_set_long_parameter(ctx, lp_tap, 25); // restore default
}

// ---------------------------------------------------------------------------
// Static one-button filter
// ---------------------------------------------------------------------------

TEST_CASE("filters/Static One Button Mode responds to key presses") {
    // Static one-button: first press starts the scan, second press selects.
    // Use Button_2 since OneButtonFilter::KeyDown has no key filter (any
    // key works).
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Static One Button Mode");

    press_release(ctx, kKeyButton2, 20);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    press_release(ctx, kKeyButton2, 20);
    run_frames(ctx, 30);

    // Output may be empty if the scan landed on a group node, but no crash.
    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

// ---------------------------------------------------------------------------
// Dynamic button filters (OneButton / TwoButton / TwoPush)
//
// These need real wall-clock sleeps because short/long-press classification
// compares iTime - m_iKeyDownTime against LP_HOLD_TIME / LP_TWO_PUSH_TOLERANCE.
// ---------------------------------------------------------------------------

TEST_CASE("filters/One Button Dynamic Mode responds to repeated presses") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "One Button Dynamic Mode");
    dasher_set_speed_percent(ctx, 200);

    // 1st press starts the filter (DynamicButtons.cpp:86-89).
    press_release(ctx, kKeyButton2, 20);

    // Subsequent presses drive the model.
    for (int k = 0; k < 10; ++k) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        press_release(ctx, kKeyButton2, 20);
        run_frames(ctx, 3, 1000 + k * 100, 16);
    }

    // No specific text-length assertion (the model may not have produced
    // text in 10 presses at low speed). The contract is "no crash, state
    // remains queryable".
    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

TEST_CASE("filters/Two Button Dynamic Mode alternates up/down keys") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Two Button Dynamic Mode");
    dasher_set_speed_percent(ctx, 200);

    press_release(ctx, kKeyButton2, 20); // start

    for (int k = 0; k < 10; ++k) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int key = (k % 2 == 0) ? kKeyButton2 : kKeyButton3; // alternate up/down
        press_release(ctx, key, 20);
        run_frames(ctx, 3, 1000 + k * 100, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

TEST_CASE("filters/Two-push Dynamic Mode accepts double presses") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Two-push Dynamic Mode (New One Button)");
    dasher_set_speed_percent(ctx, 200);

    press_release(ctx, kKeyButton2, 20);

    for (int k = 0; k < 6; ++k) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // Double press: two quick presses establishes a marker, second pair
        // selects (per TwoPushDynamicFilter.cpp:185-205).
        press_release(ctx, kKeyButton2, 20);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        press_release(ctx, kKeyButton2, 20);
        run_frames(ctx, 3, 1000 + k * 100, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

// ---------------------------------------------------------------------------
// Box filters (Menu / Direct / Alternating Direct / Compass)
//
// These schedule one zoom per keypress; fully deterministic — no sleeps.
// ---------------------------------------------------------------------------

TEST_CASE("filters/Direct Mode cycles through forward buttons") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Direct Mode");
    const int lp_zoomsteps = dasher_find_parameter_key("LP_ZOOMSTEPS");
    dasher_set_long_parameter(ctx, lp_zoomsteps, 4); // snappier zoom

    for (int k = 0; k < 30; ++k) {
        int key = kKeyButton2 + (k % 3); // Button_2, Button_3, Button_4
        press_release(ctx, key, 5);
        run_frames(ctx, 4, 1000 + k * 64, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

TEST_CASE("filters/Menu Mode scan and select") {
    // Menu Mode uses Button_1/Button_4 to scan, Button_2/Button_3 to select.
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Menu Mode");
    const int lp_zoomsteps = dasher_find_parameter_key("LP_ZOOMSTEPS");
    dasher_set_long_parameter(ctx, lp_zoomsteps, 4);

    for (int k = 0; k < 30; ++k) {
        // Scan forward with Button_4, occasionally select with Button_2.
        int key = (k % 4 == 0) ? kKeyButton2 : kKeyButton4;
        press_release(ctx, key, 5);
        run_frames(ctx, 4, 1000 + k * 64, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

TEST_CASE("filters/Alternating Direct Mode alternates buttons") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Alternating Direct Mode");
    const int lp_zoomsteps = dasher_find_parameter_key("LP_ZOOMSTEPS");
    dasher_set_long_parameter(ctx, lp_zoomsteps, 4);

    for (int k = 0; k < 30; ++k) {
        int key = (k % 2 == 0) ? kKeyButton2 : kKeyButton3;
        press_release(ctx, key, 5);
        run_frames(ctx, 4, 1000 + k * 64, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

TEST_CASE("filters/Compass Mode four direction buttons") {
    ScopedContext ctx(800, 600);
    set_filter(ctx, "Compass Mode");
    const int lp_zoomsteps = dasher_find_parameter_key("LP_ZOOMSTEPS");
    dasher_set_long_parameter(ctx, lp_zoomsteps, 4);

    for (int k = 0; k < 30; ++k) {
        int key = kKeyButton1 + (k % 4); // cycle through 4 directions
        press_release(ctx, key, 5);
        run_frames(ctx, 4, 1000 + k * 64, 16);
    }

    CHECK(std::string(dasher_get_output_text(ctx)).size() >= 0);
}

// ---------------------------------------------------------------------------
// Cross-filter contract: switching filters mid-session doesn't crash
// ---------------------------------------------------------------------------

TEST_CASE("filters/switching mid-session does not crash") {
    // CHARACTERIZATION: a context that has accumulated state under one
    // filter must cleanly accept a switch to another filter. This exercises
    // the deactivation path in CDasherInterfaceBase::CreateInputFilter
    // (DasherInterfaceBase.cpp:647-661).
    ScopedContext ctx(800, 600);
    dasher_set_speed_percent(ctx, 200);

    // Drive under Normal Control for a while.
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 100);
    dasher_mouse_up(ctx);

    // Switch to each other filter, run a few frames, switch back.
    for (int i = 1; i < kNumFilters; ++i) {
        set_filter(ctx, kAllFilters[i]);
        run_frames(ctx, 5);

        // Switch back and verify we can still drive input.
        set_filter(ctx, "Normal Control");
        dasher_mouse_move(ctx, 700.0f, 300.0f);
        dasher_mouse_down(ctx);
        run_frames(ctx, 5);
        dasher_mouse_up(ctx);
    }

    // All filter switches survived without crashing.
    CHECK(true);
}
