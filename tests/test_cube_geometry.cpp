// test_cube_geometry.cpp
//
// REGRESSION TESTS for the cube node-shape (LP_SHAPE_TYPE == CUBE, 6) via the
// C API command buffer.
//
// Bug (issue #47): CUBE mode rendered a black canvas because CDasherViewSquare
// drives cube drawing through Screen()->DrawCube / Draw3DLabel /
// DrawProjectedRectangle / FinishRender3D, and the C API's CommandScreen
// (CAPI.cpp) never overrode them — so zero draw commands were emitted for cube
// nodes. Only the clear-screen (opcode 0) fired.
//
// These tests confirm cube mode now emits real geometry (filled-rect and text
// commands) instead of just the background clear.

#include "test_common.h"

namespace {

const int LP_SHAPE_TYPE_CUBE = 6; // Options::CUBE

// Runs `frames` frames in cube mode and returns the count of the given opcode.
int count_opcode(dasher_ctx* ctx, int frames, int opcode) {
    int seen = 0;
    for (int i = 0; i < frames; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &strs, &sc);
        for (int j = 0; j + 5 < cc; j += 6) {
            if (cmds[j] == opcode) ++seen;
        }
    }
    return seen;
}

} // namespace

TEST_CASE("cube/first frame emits node geometry, not just clear-screen") {
    ScopedContext ctx(800, 600);
    const int lp_shape = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(lp_shape > 0);
    dasher_set_long_parameter(ctx, lp_shape, LP_SHAPE_TYPE_CUBE);
    REQUIRE(dasher_get_long_parameter(ctx, lp_shape) == LP_SHAPE_TYPE_CUBE);

    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);

    // Before the fix, only the clear-screen (opcode 0) plus a single decorative
    // rect/line were emitted — cube node faces never reached the buffer.
    // The root node alone produces several filled rects once DrawCube translates
    // to opcode 4.
    CHECK(count_opcode(ctx, 3, 4) > 3); // cube node faces (opcode 4)

    dasher_set_long_parameter(ctx, lp_shape, 1); // restore
}

TEST_CASE("cube/forward driving draws nodes and labels") {
    ScopedContext ctx(800, 600);
    const int lp_shape = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(lp_shape > 0);
    dasher_set_long_parameter(ctx, lp_shape, LP_SHAPE_TYPE_CUBE);

    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);

    const int rects = count_opcode(ctx, 30, 4);  // filled rectangles (cube faces)
    const int labels = count_opcode(ctx, 30, 5); // text labels (Draw3DLabel)

    dasher_mouse_up(ctx);

    // Buggy code: ~1 decorative rect/frame and ZERO labels. Fixed code draws a
    // cube face per visible node plus a label per labelled node.
    CHECK(rects > 50);
    CHECK(labels > 5);

    dasher_set_long_parameter(ctx, lp_shape, 1); // restore
}

TEST_CASE("cube/switching into and out of cube mode is stable") {
    ScopedContext ctx(800, 600);
    const int lp_shape = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(lp_shape > 0);

    dasher_set_long_parameter(ctx, lp_shape, LP_SHAPE_TYPE_CUBE);
    run_frames(ctx, 5);
    CHECK(dasher_get_long_parameter(ctx, lp_shape) == 6);

    dasher_set_long_parameter(ctx, lp_shape, 1);
    run_frames(ctx, 5);
    CHECK(dasher_get_long_parameter(ctx, lp_shape) == 1);
}
