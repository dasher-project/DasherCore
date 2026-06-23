// test_view_geometry.cpp
//
// CHARACTERIZATION TESTS for the DasherView coordinate transforms and the
// parameters that affect them: LP_ORIENTATION, BP_NONLINEAR_Y, LP_GEOMETRY.
//
// These tests use only the C API coordinate-transform entry points
// (dasher_screen_to_dasher / dasher_dasher_to_screen) plus draw-command
// inspection. They do not assert that any rendering is "correct" — they
// document that the parameters DO have an observable effect, so a future
// refactor that breaks the wiring shows up here.
//
// Reference: DasherViewSquare.cpp (Screen2Dasher L983, Dasher2Screen L1111,
// ymap/iymap L1308-1326, ComputeScaleFactor L1014-1089).

#include "test_common.h"

#include <climits>
#include <cfloat>

#include <cmath>
#include <string>

namespace {

// Aggregate stats over the rectangles (op 4) in one frame.
struct RectStats {
    int count = 0;
    long long sum_cx = 0;
    long long sum_cy = 0;
    int min_x = INT_MAX;
    int max_x = 0;
    int min_y = INT_MAX;
    int max_y = 0;
};

RectStats capture_rect_stats(dasher_ctx* ctx, int frames_to_capture) {
    RectStats stats;
    for (int i = 0; i < frames_to_capture; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &strs, &sc);
        for (int j = 0; j + 5 < cc; j += 6) {
            if (cmds[j] != 4) continue; // filled rect
            int x1 = cmds[j + 1], y1 = cmds[j + 2];
            int x2 = cmds[j + 3], y2 = cmds[j + 4];
            stats.count++;
            stats.sum_cx += (x1 + x2) / 2;
            stats.sum_cy += (y1 + y2) / 2;
            stats.min_x = std::min(stats.min_x, std::min(x1, x2));
            stats.max_x = std::max(stats.max_x, std::max(x1, x2));
            stats.min_y = std::min(stats.min_y, std::min(y1, y2));
            stats.max_y = std::max(stats.max_y, std::max(y1, y2));
        }
    }
    return stats;
}

} // namespace

// ---------------------------------------------------------------------------
// LP_ORIENTATION: 0=LeftToRight, 1=RightToLeft, 2=TopToBottom, 3=BottomToTop
// ---------------------------------------------------------------------------

TEST_CASE("view/orientation parameter defaults to AlphabetDefault") {
    // CHARACTERIZATION: LP_ORIENTATION is stored as -2 (AlphabetDefault)
    // even after realize. The runtime resolves -2 to the alphabet's own
    // orientation (LeftToRight for English) via ComputeOrientation
    // (DasherInterfaceBase.cpp:519-527), but the STORED value remains -2.
    // This means reading LP_ORIENTATION via the C API returns -2, not the
    // resolved value.
    ScopedContext ctx(800, 600);
    const int lp_orient = dasher_find_parameter_key("LP_ORIENTATION");
    REQUIRE(lp_orient > 0);
    long v = dasher_get_long_parameter(ctx, lp_orient);
    CHECK(v == -2); // AlphabetDefault — runtime resolves to alphabet's own
}

TEST_CASE("view/orientation swap is observable via screen_to_dasher") {
    // For LeftToRight on an 800x600 canvas:
    //   screen X near right (sx=700) -> DasherX is small (forward)
    //   screen Y (sy=300)            -> DasherY derived linearly
    // For TopToBottom the screen X and Y roles swap:
    //   screen X (sx=700) now feeds DasherY (not DasherX)
    // So for the same (sx, sy), the (dx, dy) outputs differ between the two
    // orientations. This is the cleanest direct assertion that orientation
    // actually changes the transform.
    ScopedContext ctx(800, 600);
    const int lp_orient = dasher_find_parameter_key("LP_ORIENTATION");

    dasher_set_long_parameter(ctx, lp_orient, 0); // LeftToRight
    long long dx_lr, dy_lr;
    REQUIRE(dasher_screen_to_dasher(ctx, 700, 300, &dx_lr, &dy_lr) == 0);

    dasher_set_long_parameter(ctx, lp_orient, 2); // TopToBottom
    long long dx_tb, dy_tb;
    REQUIRE(dasher_screen_to_dasher(ctx, 700, 300, &dx_tb, &dy_tb) == 0);

    // On a non-square canvas (800x600), the axes-swap MUST produce
    // different outputs.
    CHECK((dx_lr != dx_tb || dy_lr != dy_tb));

    dasher_set_long_parameter(ctx, lp_orient, 0); // restore
}

TEST_CASE("view/orientation each value is reachable") {
    // Smoke test: each of the 4 orientations accepts, stores, and returns
    // the value, and the transform still returns a finite result.
    ScopedContext ctx(800, 600);
    const int lp_orient = dasher_find_parameter_key("LP_ORIENTATION");

    for (long orient : {0L, 1L, 2L, 3L}) {
        dasher_set_long_parameter(ctx, lp_orient, orient);
        CHECK(dasher_get_long_parameter(ctx, lp_orient) == orient);

        long long dx, dy;
        REQUIRE(dasher_screen_to_dasher(ctx, 400, 300, &dx, &dy) == 0);
        // The transform must return a sensible dasher-space value.
        // Dasher coordinates are non-negative integers up to ~4096.
        CHECK(dx >= -100000);
        CHECK(dx <= 100000);
        CHECK(dy >= -100000);
        CHECK(dy <= 100000);
    }

    dasher_set_long_parameter(ctx, lp_orient, 0);
}

TEST_CASE("view/root child bounds are invariant under orientation") {
    // CHARACTERIZATION: dasher_get_root_child_bounds returns dasher-space
    // probabilities stored on the node tree. They must NOT change with
    // orientation — orientation only changes how those bounds are *drawn*.
    ScopedContext ctx(800, 600);
    const int lp_orient = dasher_find_parameter_key("LP_ORIENTATION");

    int n = dasher_get_root_child_count(ctx);
    REQUIRE(n > 0);

    std::vector<long long> lbnds_lr(n), hbnds_lr(n);
    for (int i = 0; i < n; ++i) {
        REQUIRE(dasher_get_root_child_bounds(ctx, i, &lbnds_lr[i], &hbnds_lr[i]) == 0);
    }

    dasher_set_long_parameter(ctx, lp_orient, 2); // TopToBottom

    for (int i = 0; i < n; ++i) {
        long long lb, hb;
        REQUIRE(dasher_get_root_child_bounds(ctx, i, &lb, &hb) == 0);
        CHECK(lb == lbnds_lr[i]);
        CHECK(hb == hbnds_lr[i]);
    }

    dasher_set_long_parameter(ctx, lp_orient, 0);
}

// ---------------------------------------------------------------------------
// BP_NONLINEAR_Y: compresses top/bottom of screen so crosshair gets more space
// ---------------------------------------------------------------------------

TEST_CASE("view/nonlinear_y default is on") {
    // CHARACTERIZATION: BP_NONLINEAR_Y defaults to true. This affects the
    // ymap function (DasherViewSquare.cpp:1308) which compresses Y values
    // outside [m_Y3, m_Y2] = [204, 3891] by a factor of 4.
    ScopedContext ctx(800, 600);
    const int bp_nly = dasher_find_parameter_key("BP_NONLINEAR_Y");
    REQUIRE(bp_nly > 0);
    CHECK(dasher_get_bool_parameter(ctx, bp_nly) == 1);
}

TEST_CASE("view/nonlinear_y changes mapping for extreme screen Y") {
    // For a screen point that maps to extreme Dasher Y (top or bottom edge),
    // toggling BP_NONLINEAR_Y must change the dasher Y output (because ymap
    // compresses the top/bottom regions when enabled).
    ScopedContext ctx(800, 600);
    const int bp_nly = dasher_find_parameter_key("BP_NONLINEAR_Y");

    // Probe near the top corner (sy=0). On 800x600 canvas this maps to
    // DasherY near 0 (compressed region when nonlinear_y is on).
    dasher_set_bool_parameter(ctx, bp_nly, 0);
    long long dx_off, dy_off;
    REQUIRE(dasher_screen_to_dasher(ctx, 400, 0, &dx_off, &dy_off) == 0);

    dasher_set_bool_parameter(ctx, bp_nly, 1);
    long long dx_on, dy_on;
    REQUIRE(dasher_screen_to_dasher(ctx, 400, 0, &dx_on, &dy_on) == 0);

    // dy must differ at the extreme; dx might be identical (X axis is
    // unaffected by nonlinear_y).
    CHECK(dy_off != dy_on);

    dasher_set_bool_parameter(ctx, bp_nly, 1); // restore default
}

TEST_CASE("view/nonlinear_y does not affect middle of screen") {
    // Points in the middle of the Y range (near MAX_Y/2 = 2048, which
    // maps to roughly the screen center) fall inside the linear band
    // [m_Y3, m_Y2] = [204, 3891]. ymap returns y unchanged for these.
    ScopedContext ctx(800, 600);
    const int bp_nly = dasher_find_parameter_key("BP_NONLINEAR_Y");

    dasher_set_bool_parameter(ctx, bp_nly, 0);
    long long dx_off, dy_off;
    REQUIRE(dasher_screen_to_dasher(ctx, 400, 300, &dx_off, &dy_off) == 0);

    dasher_set_bool_parameter(ctx, bp_nly, 1);
    long long dx_on, dy_on;
    REQUIRE(dasher_screen_to_dasher(ctx, 400, 300, &dx_on, &dy_on) == 0);

    // For a center point, the difference must be tiny (rounding only).
    CHECK(std::llabs(dy_off - dy_on) <= 2);

    dasher_set_bool_parameter(ctx, bp_nly, 1);
}

// ---------------------------------------------------------------------------
// LP_GEOMETRY: 0=old_style, 1=square_no_xhair, 2=squish, 3=squish_and_log
// ---------------------------------------------------------------------------

TEST_CASE("view/geometry default is old_style") {
    ScopedContext ctx(800, 600);
    const int lp_geom = dasher_find_parameter_key("LP_GEOMETRY");
    REQUIRE(lp_geom > 0);
    CHECK(dasher_get_long_parameter(ctx, lp_geom) == 0);
}

TEST_CASE("view/geometry affects X mapping for large dx") {
    // CHARACTERIZATION: ComputeScaleFactor (DasherViewSquare.cpp:1014) is
    // called when LP_GEOMETRY changes, deriving iScaleFactorX/Y. For a
    // dasher X far from the crosshair (dx close to 4096), the screen X
    // output should differ between geometry modes.
    ScopedContext ctx(800, 600);
    const int lp_geom = dasher_find_parameter_key("LP_GEOMETRY");

    int sx_old, sy_old;
    dasher_set_long_parameter(ctx, lp_geom, 0); // old_style
    REQUIRE(dasher_dasher_to_screen(ctx, 3800, 2048, &sx_old, &sy_old) == 0);

    int sx_squish, sy_squish;
    dasher_set_long_parameter(ctx, lp_geom, 3); // squish_and_log
    REQUIRE(dasher_dasher_to_screen(ctx, 3800, 2048, &sx_squish, &sy_squish) == 0);

    // X mapping must differ between the two geometry modes for far-dx.
    CHECK(std::abs(sx_old - sx_squish) > 0);

    dasher_set_long_parameter(ctx, lp_geom, 0);
}

// ---------------------------------------------------------------------------
// End-to-end rendering: orientation affects draw-command coordinates
// ---------------------------------------------------------------------------

TEST_CASE("view/rendering rect centroids differ across orientations") {
    // CHARACTERIZATION: with a non-square canvas (800x600), the bounding-box
    // centroid of all filled-rectangle draw commands (opcode 4) differs
    // between LeftToRight and TopToBottom. This is the strongest end-to-end
    // assertion that orientation actually affects what's drawn.
    auto run_scenario = [](long orientation) {
        ScopedContext ctx(800, 600);
        const int lp_orient = dasher_find_parameter_key("LP_ORIENTATION");
        dasher_set_long_parameter(ctx, lp_orient, orientation);

        // Drive the same scenario: hover at a fixed point to grow nodes.
        dasher_set_speed_percent(ctx, 200);
        dasher_mouse_move(ctx, 700.0f, 300.0f);
        dasher_mouse_down(ctx);
        RectStats stats = capture_rect_stats(ctx, 30);
        dasher_mouse_up(ctx);
        return stats;
    };

    RectStats lr = run_scenario(0);
    RectStats tb = run_scenario(2);

    REQUIRE(lr.count > 0);
    REQUIRE(tb.count > 0);

    // Mean centroid must differ. (X and Y roles swap; on a non-square
    // canvas this can't be a no-op.)
    long mean_cx_lr = lr.sum_cx / lr.count;
    long mean_cy_lr = lr.sum_cy / lr.count;
    long mean_cx_tb = tb.sum_cx / tb.count;
    long mean_cy_tb = tb.sum_cy / tb.count;
    CHECK(std::abs(mean_cx_lr - mean_cx_tb) + std::abs(mean_cy_lr - mean_cy_tb) > 5);
}
