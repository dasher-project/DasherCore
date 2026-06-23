// test_control_actions_navigation.cpp
//
// CHARACTERIZATION TEST for control-mode action firing through actual node
// navigation. Closes the explicit gap at test_control_actions.cpp:250-253:
//
//   "The callback won't fire just from parsing — it fires when a node
//    containing the action is navigated into. We've verified the registration
//    doesn't crash and the context works with control mode enabled.
//    The ActionRegistry unit tests above verify callback firing directly."
//
// What this test adds: the C-side callback actually fires when the user
// navigates into the control node that references the registered action.
//
// Mechanism:
//   1. Build a temp Data/ dir with a custom control.xml that references
//      our action name.
//   2. dasher_register_action() — adds factory to ActionRegistry.
//   3. Toggle BP_CONTROL_MODE on — triggers CreateControlBox, which calls
//      GetPendingCustomActions() (drains ctx->customActions into the new
//      control manager's registry) and parses our control.xml.
//   4. Drive the mouse into the control child's screen region.
//   5. CContNode::Do() fires executeActions, which calls our callback.

#include "test_common.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <thread>

namespace {

// Captured callback state. Static so the C-linkage callback can find it.
// Atomic because the engine may eventually call from a different thread
// (it doesn't today, but defensive).
struct CallbackCapture {
    std::atomic<int> call_count{0};
    std::string last_name;
    std::map<std::string, std::string> last_attrs;
};
CallbackCapture g_capture;

void reset_capture() {
    g_capture.call_count.store(0);
    g_capture.last_name.clear();
    g_capture.last_attrs.clear();
}

} // namespace

// The C callback signature (from dasher.h).
extern "C" void test_action_callback(const char* name, int attr_count, const char** keys, const char** values,
                                     void* user_data) {
    (void)user_data;
    g_capture.call_count.fetch_add(1);
    g_capture.last_name = name ? name : "";
    g_capture.last_attrs.clear();
    for (int i = 0; i < attr_count; ++i) {
        g_capture.last_attrs[keys[i] ? keys[i] : ""] = values[i] ? values[i] : "";
    }
}

// ---------------------------------------------------------------------------
// Foundational: control mode adds a top-level child to the crosshair node
// ---------------------------------------------------------------------------

TEST_CASE("ctrl/enabling control mode adds a top-level child") {
    // CHARACTERIZATION: turning on BP_CONTROL_MODE grafts the control tree
    // as an extra top-level child at the end of the crosshair's children.
    // Verified by counting children before and after.
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);
    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    const int bp_cm = dasher_find_parameter_key("BP_CONTROL_MODE");
    REQUIRE(bp_cm > 0);

    int n_off = dasher_get_root_child_count(ctx);

    dasher_set_bool_parameter(ctx, bp_cm, 1);
    // Run one frame so the new node tree is realized.
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);

    int n_on = dasher_get_root_child_count(ctx);
    CHECK(n_on == n_off + 1);

    // The new (last) child has bounds ~= [62259, 65536] (control gets
    // NORMALIZATION/20 = 3277 of the 65536 total).
    long long lb = 0, hb = 0;
    REQUIRE(dasher_get_root_child_bounds(ctx, n_on - 1, &lb, &hb) == 0);
    CHECK(hb == 65536);
    CHECK(hb - lb <= 65536 / 10); // generous upper bound on control share

    dasher_set_bool_parameter(ctx, bp_cm, 0);
    dasher_destroy(ctx);
}

// ---------------------------------------------------------------------------
// The actual callback firing through navigation
// ---------------------------------------------------------------------------

TEST_CASE("ctrl/callback fires when user navigates into control node") {
    // Reset state.
    reset_capture();

    // Build a temp Data dir and replace control.xml with one that references
    // our action.
    ScopedTempDir tmp;
    std::string data_dir = build_data_dir(tmp);

    // Remove the symlink to the bundled control.xml, replace with our custom
    // version that adds a node referencing our test action.
    std::filesystem::path ctl_path = std::filesystem::path(data_dir) / "Data" / "control" / "control.xml";
    std::error_code ec;
    std::filesystem::remove(ctl_path, ec);

    const std::string custom_control_xml =
        "<?xml version=\"1.0\"?>\n"
        "<control name=\"\">\n"
        "  <alph/>\n"
        "  <root/>\n"
        "  <node label=\"FireCallback\" color=\"240\">\n"
        "    <test_action endpoint=\"/api/test\" method=\"POST\" priority=\"high\"/>\n"
        "    <alph/>\n"
        "    <root/>\n"
        "  </node>\n"
        "</control>\n";
    REQUIRE(write_data_file(data_dir, "control", "control.xml", custom_control_xml));

    dasher_ctx* ctx = dasher_create(data_dir.c_str(), tmp.c_str(), nullptr);
    REQUIRE(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // Register the action BEFORE enabling control mode. This puts it in the
    // pending list, which CreateControlBox drains when BP_CONTROL_MODE flips.
    dasher_register_action(ctx, "test_action", test_action_callback, nullptr);

    // Enable control mode. This rebuilds the control box, picking up our
    // pending action and parsing the custom control.xml (which attaches
    // <test_action/> to the FireCallback node).
    const int bp_cm = dasher_find_parameter_key("BP_CONTROL_MODE");
    dasher_set_bool_parameter(ctx, bp_cm, 1);

    // Disable slow-control-box so the navigation is fast enough for the test.
    const int bp_slow = dasher_find_parameter_key("BP_SLOW_CONTROL_BOX");
    if (bp_slow > 0) {
        dasher_set_bool_parameter(ctx, bp_slow, 0);
    }

    // Run one frame to populate root children.
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);

    // Sanity: control mode is on, control child exists at the end.
    const int n = dasher_get_root_child_count(ctx);
    REQUIRE(n > 0);
    long long lb, hb;
    REQUIRE(dasher_get_root_child_bounds(ctx, n - 1, &lb, &hb) == 0);

    // Aim at the control child's screen-Y midpoint. The control child sits
    // near the edge of the screen (its bounds are ~[62259, 65536] of
    // 65536, so its midpoint fraction is ~0.95). On a 600-tall screen
    // that's sy ~= 570. Use 580 to land solidly inside the control band
    // while staying on-screen.
    dasher_set_speed_percent(ctx, 300);
    const float sx = 700.0f;
    const float sy = 580.0f;
    dasher_mouse_move(ctx, sx, sy);
    dasher_mouse_down(ctx);

    // Drive frames until the callback fires or we time out. Budget ~600
    // frames (~10 seconds at 60 FPS) — control nodes need time to descend
    // into.
    const int frame_budget = 600;
    int frames_run = 0;
    for (; frames_run < frame_budget; ++frames_run) {
        dasher_mouse_move(ctx, sx, sy);
        int* cmds2 = nullptr;
        int cc2 = 0;
        char** strs2 = nullptr;
        int sc2 = 0;
        dasher_frame(ctx, 1000 + frames_run * 16, &cmds2, &cc2, &strs2, &sc2);
        if (g_capture.call_count.load() > 0) break;
    }
    dasher_mouse_up(ctx);

    // The callback must have fired at least once.
    INFO("callback fired ", g_capture.call_count.load(), " times after ", frames_run, " frames");
    CHECK(g_capture.call_count.load() >= 1);

    // And the attributes must match what we put in control.xml.
    if (g_capture.call_count.load() > 0) {
        CHECK(g_capture.last_name == std::string("test_action"));
        CHECK(g_capture.last_attrs["endpoint"] == "/api/test");
        CHECK(g_capture.last_attrs["method"] == "POST");
        CHECK(g_capture.last_attrs["priority"] == "high");
    }

    dasher_destroy(ctx);
    reset_capture();
}
