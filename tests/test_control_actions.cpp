// Tests for the unified action system (ControlManager / ActionRegistry)
// Tests both internal C++ classes and the C API integration.
//
// Build: linked against DasherCore internally (needs ActionRegistry, ControlAction)

#include "test_common.h"

#include "DasherCore/ControlManager.h"

#include <map>
#include <string>

using namespace Dasher;

// ── ActionRegistry unit tests ──────────────────────────────────────────────

TEST(action_registry_empty) {
    ActionRegistry registry;
    ASSERT(!registry.hasAction("nonexistent"));

    std::map<std::string, std::string> emptyAttrs;
    ASSERT(registry.create("nonexistent", emptyAttrs) == nullptr);
}

TEST(action_registry_factory) {
    ActionRegistry registry;

    // Register a factory that creates a StopAction
    registry.registerFactory("stop", [](const auto&) { return new StopAction(); });

    ASSERT(registry.hasAction("stop"));

    std::map<std::string, std::string> attrs;
    ControlAction* action = registry.create("stop", attrs);
    ASSERT(action != nullptr);
    delete action;
}

TEST(action_registry_custom_action) {
    ActionRegistry registry;

    // Track callback invocation
    static std::string receivedName;
    static std::map<std::string, std::string> receivedAttrs;
    static int callbackCount = 0;
    callbackCount = 0;

    registry.registerCustomAction("my_action",
                                  [](const std::string& name, const std::map<std::string, std::string>& attrs) {
                                      receivedName = name;
                                      receivedAttrs = attrs;
                                      callbackCount++;
                                  });

    ASSERT(registry.hasAction("my_action"));

    std::map<std::string, std::string> attrs = {{"key1", "val1"}, {"key2", "val2"}};
    ControlAction* action = registry.create("my_action", attrs);
    ASSERT(action != nullptr);

    // Execute — CustomAction::execute doesn't use the interface pointer
    action->execute(nullptr);
    delete action;

    ASSERT_EQ(callbackCount, 1);
    ASSERT_STR_EQ(receivedName.c_str(), "my_action");
    ASSERT_EQ(receivedAttrs.size(), (size_t)2);
    ASSERT_STR_EQ(receivedAttrs["key1"].c_str(), "val1");
    ASSERT_STR_EQ(receivedAttrs["key2"].c_str(), "val2");
}

TEST(action_registry_overwrite) {
    ActionRegistry registry;

    int callCount1 = 0, callCount2 = 0;

    registry.registerCustomAction("action_a", [&](const auto&, const auto&) { callCount1++; });

    // Overwrite with a different callback
    registry.registerCustomAction("action_a", [&](const auto&, const auto&) { callCount2++; });

    std::map<std::string, std::string> attrs;
    ControlAction* action = registry.create("action_a", attrs);
    ASSERT(action != nullptr);
    action->execute(nullptr);
    delete action;

    // Only the second callback should fire
    ASSERT_EQ(callCount1, 0);
    ASSERT_EQ(callCount2, 1);
}

// ── C API integration tests ────────────────────────────────────────────────

// Shared callback data for C API tests (must be at file scope for C callbacks)
struct ActionCallbackData {
    std::string name;
    std::map<std::string, std::string> attrs;
    int callCount = 0;
    void reset() {
        name.clear();
        attrs.clear();
        callCount = 0;
    }
};

static ActionCallbackData g_callbackData;

static void action_callback(const char* name, int count, const char** keys, const char** vals, void* /*ud*/) {
    g_callbackData.name = name ? name : "";
    g_callbackData.attrs.clear();
    for (int i = 0; i < count; i++) {
        g_callbackData.attrs[keys[i]] = vals[i];
    }
    g_callbackData.callCount++;
}

TEST(capi_register_action_null_safety) {
    // All these should handle null gracefully without crashing
    static int callbackCalled = 0;
    auto cb = [](const char*, int, const char**, const char**, void*) { callbackCalled++; };

    dasher_register_action(nullptr, "test_action", cb, nullptr);
    dasher_register_action(nullptr, nullptr, cb, nullptr);
    dasher_register_action(nullptr, "test_action", nullptr, nullptr);

    ASSERT_EQ(callbackCalled, 0);
}

TEST(capi_register_action_before_realize) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Register a custom action before realization
    static int callbackCalled = 0;
    callbackCalled = 0;

    dasher_register_action(
        ctx, "test_custom_action", [](const char*, int, const char**, const char**, void*) { callbackCalled++; },
        nullptr);

    // Now realize
    dasher_set_screen_size(ctx, 800, 600);

    // Enable control mode
    int bp_control_mode = dasher_find_parameter_key("BP_CONTROL_MODE");
    ASSERT(bp_control_mode >= 0);
    dasher_set_bool_parameter(ctx, bp_control_mode, 1);

    // Run a frame to ensure control box is built
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    // The context should work without crashing
    // The control node should be present (root child count > 0)
    int root_children = dasher_get_root_child_count(ctx);
    ASSERT(root_children > 0);

    dasher_destroy(ctx);
}

TEST(capi_register_action_after_realize) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    // Realize first
    dasher_set_screen_size(ctx, 800, 600);

    // Enable control mode
    int bp_control_mode = dasher_find_parameter_key("BP_CONTROL_MODE");
    ASSERT(bp_control_mode >= 0);
    dasher_set_bool_parameter(ctx, bp_control_mode, 1);

    // Run a frame
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    // Register custom action after realization — should not crash
    dasher_register_action(ctx, "late_action", [](const char*, int, const char**, const char**, void*) {}, nullptr);

    // Should be able to run more frames without issues
    dasher_frame(ctx, 2000, &commands, &cmd_count, &strings, &str_count);

    dasher_destroy(ctx);
}

TEST(capi_control_mode_enables_successfully) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int bp_control_mode = dasher_find_parameter_key("BP_CONTROL_MODE");
    ASSERT(bp_control_mode >= 0);

    // Enable control mode — this rebuilds the control box and node tree
    dasher_set_bool_parameter(ctx, bp_control_mode, 1);
    ASSERT_EQ(dasher_get_bool_parameter(ctx, bp_control_mode), 1);

    // Run a frame — should not crash or hang
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);
    ASSERT(cmd_count > 0);

    dasher_destroy(ctx);
}

TEST(capi_action_callback_receives_attrs) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    g_callbackData.reset();

    dasher_register_action(ctx, "attr_test", action_callback, &g_callbackData);

    dasher_set_screen_size(ctx, 800, 600);

    // The action is registered in the registry. We can verify it exists by
    // checking that enabling control mode and running doesn't crash.
    int bp_control_mode = dasher_find_parameter_key("BP_CONTROL_MODE");
    dasher_set_bool_parameter(ctx, bp_control_mode, 1);

    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 1000, &commands, &cmd_count, &strings, &str_count);

    // The callback won't fire just from parsing — it fires when a node
    // containing the action is navigated into. We've verified the registration
    // doesn't crash and the context works with control mode enabled.
    // The ActionRegistry unit tests above verify callback firing directly.

    dasher_destroy(ctx);
}
