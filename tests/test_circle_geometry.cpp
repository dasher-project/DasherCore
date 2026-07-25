// test_circle_geometry.cpp
//
// REGRESSION TESTS for the circle node-shape (LP_SHAPE_TYPE == CIRCLE, 5).
//
// Two crashes historically affected the circular/compass Dasher layout, both
// in CDasherViewSquare (src/DasherCore/DasherViewSquare.cpp):
//
//   Bug 1 (SIGABRT): IsSpaceAroundNode() asserts CoversCrosshair(...) for any
//     node that spans the visible region. That invariant only holds for
//     rectangular shapes — a circle/ellipse can span the full height yet
//     legitimately miss the crosshair, so the assert is unsound for circles
//     (and triangles/quadrics).
//
//   Bug 2 (SIGSEGV / stack overflow): CircleTo() recursively subdivides an arc
//     with no recursion-depth bound. Degenerate endpoints (integer-rounding
//     stall, or sqrt of a negative -> NaN) prevent the convergence test from
//     ever firing and it recurses until the stack is exhausted.
//
// These tests drive normal rendering with LP_SHAPE_TYPE=CIRCLE. If either bug
// is present, the test *process* is killed (abort/segv), so simply reaching
// the assertions is the proof of the fix. We additionally check that circle
// geometry actually emits polygon (line-segment) draw commands.

#include "test_common.h"

#include <cmath>

namespace {

// LP_SHAPE_TYPE key (Options::CIRCLE == 5). Resolved lazily inside each test,
// NOT at static init — dasher_find_parameter_key needs the engine registered,
// and doctest's context isn't available until main() runs.
static int shape_type_key() {
    const int k = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(k > 0);
    return k;
}

// Counts opcode-2 (Line) segments emitted across `frames` frames. The C-API
// screen renders Polygon() (used by the circle shape) as a fan of opcode-2
// line segments, so this is a proxy for "circle shapes were actually drawn".
int count_line_segments(dasher_ctx* ctx, int frames) {
    int lines = 0;
    for (int i = 0; i < frames; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &strs, &sc);
        for (int j = 0; j + 5 < cc; j += 6) {
            if (cmds[j] == 2) ++lines;
        }
    }
    return lines;
}

} // namespace

// ---------------------------------------------------------------------------
// Bug 1 + Bug 2: rendering a single frame in circle mode must not abort/crash.
// ---------------------------------------------------------------------------

TEST_CASE("circle/first frame does not crash") {
    ScopedContext ctx(800, 600);
    dasher_set_long_parameter(ctx, shape_type_key(), 5); // Options::CIRCLE

    // A single frame calls NewRender -> IsSpaceAroundNode (Bug 1) and, for
    // nodes drawn as circles, Circle -> CircleTo (Bug 2).
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);

    // The frame must have produced *some* drawing.
    CHECK(cc > 0);

    dasher_set_long_parameter(ctx, shape_type_key(), 1); // restore default
}

// ---------------------------------------------------------------------------
// Bug 2: deep recursion. Drive the engine forward so nodes grow small, which
// makes the per-node circles shrink and stresses CircleTo's subdivision.
// ---------------------------------------------------------------------------

TEST_CASE("circle/many forward frames do not overflow the stack") {
    ScopedContext ctx(800, 600);
    dasher_set_long_parameter(ctx, shape_type_key(), 5); // Options::CIRCLE

    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);

    // Plenty of frames at a high bit-rate to force deep node trees / small
    // circles. If CircleTo recurses without bound this segfaults.
    const int lines = count_line_segments(ctx, 60);

    dasher_mouse_up(ctx);

    CHECK(lines > 0); // circle geometry actually drew something

    dasher_set_long_parameter(ctx, shape_type_key(), 1); // restore default
}

// ---------------------------------------------------------------------------
// The setting must be switchable at runtime without leaving the engine in a
// bad state (covers the parameter-change -> ComputeScaleFactor path).
// ---------------------------------------------------------------------------

TEST_CASE("circle/switching into and out of circle mode is stable") {
    ScopedContext ctx(800, 600);

    dasher_set_long_parameter(ctx, shape_type_key(), 5);
    run_frames(ctx, 5);
    CHECK(dasher_get_long_parameter(ctx, shape_type_key()) == 5);

    dasher_set_long_parameter(ctx, shape_type_key(), 1);
    run_frames(ctx, 5);
    CHECK(dasher_get_long_parameter(ctx, shape_type_key()) == 1);

    // back to circle, then back to default, all while ticking frames
    dasher_set_long_parameter(ctx, shape_type_key(), 5);
    run_frames(ctx, 5);
    dasher_set_long_parameter(ctx, shape_type_key(), 1);
    run_frames(ctx, 5);
}
