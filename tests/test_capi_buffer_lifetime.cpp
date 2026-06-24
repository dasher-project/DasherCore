// test_capi_buffer_lifetime.cpp
//
// CHARACTERIZATION TESTS for the C API pointer-lifetime contract documented
// in dasher.h. The contract reads (dasher.h:94):
//
//   "All returned pointers are valid only until the next dasher_frame() call
//    on the same context. Do not free them."
//
// Before now, this contract was not asserted anywhere. These tests document
// the current behavior so a future change that breaks the contract is
// immediately visible. They are not "is this the right contract?" tests —
// they are "is the contract being honored?" tests.

#include "test_common.h"

#include <cstring>
#include <string>

// ---------------------------------------------------------------------------
// dasher_frame output pointers
//
// The contract: pointers returned via out_parameters are valid until the
// next call to dasher_frame() on the same context. After the next call they
// may have been reallocated, reused, or invalidated.
// ---------------------------------------------------------------------------

TEST_CASE("frame/pointers valid immediately after call") {
    ScopedContext ctx(800, 600);

    int* cmds = nullptr;
    int cmd_count = 0;
    char** strs = nullptr;
    int str_count = 0;

    dasher_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);

    // Pointers must be non-null for any non-trivial frame (cmd_count > 0).
    // First frame should at minimum contain a clear-screen command.
    REQUIRE(cmds != nullptr);
    REQUIRE(cmd_count >= 6); // at least one 6-int command
    // strs pointer may be null if str_count == 0 (no text on first frame).
    // Only assert non-null when there are actual strings.
    if (str_count > 0) {
        REQUIRE(strs != nullptr);
    }

    // Reading through the pointers immediately must not crash.
    int first_opcode = cmds[0];
    CHECK(first_opcode >= 0);
    CHECK(first_opcode <= 6);

    // We can iterate all commands without faulting.
    int opcodes_seen[7] = {0};
    for (int i = 0; i < cmd_count; i += 6) {
        int op = cmds[i];
        if (op >= 0 && op <= 6) opcodes_seen[op]++;
    }
    CHECK(opcodes_seen[0] >= 1); // clear-screen is always issued
}

TEST_CASE("frame/output_text pointer stable until next call") {
    ScopedContext ctx(800, 600);

    // Drive some input so the output text becomes non-empty.
    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 200);

    const char* text1 = dasher_get_output_text(ctx);
    REQUIRE(text1 != nullptr);
    const size_t len1 = std::string(text1).size();
    if (len1 == 0) {
        // If hover-at-(700,300) didn't produce text on this platform, the
        // pointer-lifetime assertion below is vacuous. Still, the contract
        // is "valid until next call" — verify by checking that the pointer
        // compares equal on a second call without an intervening frame.
        const char* text1_again = dasher_get_output_text(ctx);
        CHECK(text1 == text1_again);
        return;
    }

    // Without an intervening frame, the pointer must be stable.
    const char* text1_again = dasher_get_output_text(ctx);
    CHECK(text1 == text1_again);

    // Run a frame, then re-read. The contract says the OLD pointer may now
    // be invalid. We cannot safely dereference text1 anymore — but we can
    // assert that the new pointer either differs OR the content matches the
    // old length (i.e. the implementation chose to reuse the buffer in
    // place, which is also contract-compliant).
    run_frames(ctx, 5);
    const char* text2 = dasher_get_output_text(ctx);
    REQUIRE(text2 != nullptr);
    // Strongest correct assertion: text2 is itself dereferenceable and
    // contains at least len1 characters (output text is append-only).
    CHECK(std::string(text2).size() >= len1);
}

TEST_CASE("frame/pointers may differ between successive frames") {
    // CHARACTERIZATION: documents that the implementation MAY return
    // different buffer pointers across successive dasher_frame calls. The
    // contract explicitly says callers must not hold pointers across calls.
    ScopedContext ctx(800, 600);

    int* cmds1 = nullptr;
    int cc1 = 0;
    char** strs1 = nullptr;
    int sc1 = 0;
    dasher_frame(ctx, 1000, &cmds1, &cc1, &strs1, &sc1);

    // Save the last opcode NOW, before the second dasher_frame() call
    // invalidates cmds1 (the contract: pointers are valid until the next
    // dasher_frame() call). Reading cmds1 after the second frame is UB.
    REQUIRE(cc1 >= 6);
    int last_op1 = cmds1[cc1 - 6];

    int* cmds2 = nullptr;
    int cc2 = 0;
    char** strs2 = nullptr;
    int sc2 = 0;
    dasher_frame(ctx, 1016, &cmds2, &cc2, &strs2, &sc2);

    // Both pointers must be valid in isolation.
    REQUIRE(cmds1 != nullptr);
    REQUIRE(cmds2 != nullptr);
    REQUIRE(cc2 >= 6);
    int last_op2 = cmds2[cc2 - 6];
    CHECK(last_op1 >= 0);
    CHECK(last_op2 >= 0);
}

TEST_CASE("frame/output_text content stable across non-frame calls") {
    // CHARACTERIZATION: dasher_get_output_text reassigns ctx->tlString on
    // every call (CAPI.cpp:791), so the returned pointer is NOT guaranteed
    // stable across successive calls. But the CONTENT must be preserved
    // across non-frame API calls (getters/setters don't change the buffer).
    // The real contract is: the content at the returned address is valid
    // (dereferenceable, correct) until the next frame call.
    ScopedContext ctx(800, 600);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 100);

    const std::string snapshot(dasher_get_output_text(ctx));
    REQUIRE(snapshot.size() > 0);

    // Perform a bunch of non-frame API calls.
    (void)dasher_get_alphabet_id(ctx);
    (void)dasher_get_speed_percent(ctx);
    (void)dasher_get_language_model_id(ctx);
    (void)dasher_get_bool_parameter(ctx, 0);
    (void)dasher_get_string_parameter(ctx, 0);
    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 705.0f, 295.0f);

    // The CONTENT must be unchanged after non-frame API calls.
    CHECK(std::string(dasher_get_output_text(ctx)) == snapshot);
}

TEST_CASE("frame/separate contexts do not share buffers") {
    // Each context owns its own buffers. Two contexts alive simultaneously
    // must not interfere with each other's pointer lifetimes.
    ScopedContext a(800, 600);
    ScopedContext b(800, 600);

    int* cmds_a = nullptr;
    int cc_a = 0;
    char** strs_a = nullptr;
    int sc_a = 0;
    int* cmds_b = nullptr;
    int cc_b = 0;
    char** strs_b = nullptr;
    int sc_b = 0;

    dasher_frame(a, 1000, &cmds_a, &cc_a, &strs_a, &sc_a);
    dasher_frame(b, 1000, &cmds_b, &cc_b, &strs_b, &sc_b);

    REQUIRE(cmds_a != nullptr);
    REQUIRE(cmds_b != nullptr);
    // The contract is per-context. Calling frame on b must not invalidate a.
    // We verify by reading cmds_a after b's frame — this must not crash and
    // must still be a sensible opcode.
    CHECK(cmds_a[0] >= 0);
    CHECK(cmds_a[0] <= 6);

    // The two contexts must not return the same buffer address.
    CHECK(cmds_a != cmds_b);
}
