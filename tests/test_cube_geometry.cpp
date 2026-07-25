// test_cube_geometry.cpp
//
// REGRESSION TESTS for the cube node-shape (LP_SHAPE_TYPE == CUBE, 6) via the
// C API command buffer.
//
// Issue #47: CUBE mode rendered a black canvas — CommandScreen never overrode
// the 3D draw hooks. Fixed by overriding DrawCube / Draw3DLabel /
// DrawProjectedRectangle.
//
// Issue #49: the initial fix rendered flat (DrawCube discarded the depth info
// and emitted one opcode-4 rect). Fixed by emitting a dedicated opcode-7
// record carrying the raw cube data (bounds + extrusionLevel + colours +
// thickness) so frontends can composite a 3D overlay in a second pass.
//
// These tests confirm (a) cube nodes reach the buffer as opcode 7, (b) labels
// reach it as opcode 5, and (c) the per-node extrusion depth survives the trip
// — i.e. the depth info is no longer discarded.

#include "test_common.h"

#include <set>

namespace {

const int LP_SHAPE_TYPE_CUBE = 6; // Options::CUBE

// Total ints consumed by the command starting at cmds[j]. Opcodes 0-6 are one
// 6-int slot; opcode 7 (Cube) occupies TWO 6-int slots (12 ints). The buffer is
// uniformly 6-int divisible; opcode 7 just spans two slots.
int cmd_size(const int* cmds, int j) {
    return cmds[j] == 7 ? 12 : 6;
}

// Counts occurrences of `opcode` across one frame's command buffer, honouring
// the opcode-7 two-slot stride.
int count_opcode_frame(const int* cmds, int cc, int opcode) {
    int seen = 0;
    for (int j = 0; j + 5 < cc;) {
        const int sz = cmd_size(cmds, j);
        if (j + sz > cc) break;
        if (cmds[j] == opcode) ++seen;
        j += sz;
    }
    return seen;
}

// Runs `frames` frames and counts occurrences of `opcode`.
int count_opcode(dasher_ctx* ctx, int frames, int opcode) {
    int seen = 0;
    for (int i = 0; i < frames; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &strs, &sc);
        seen += count_opcode_frame(cmds, cc, opcode);
    }
    return seen;
}

// Collects every extrusionLevel value (field at offset 5 of an opcode-7 record)
// seen across `frames` frames. Used to verify the depth data is carried, not
// flattened to a single value.
std::set<int> collect_extrusion_levels(dasher_ctx* ctx, int frames) {
    std::set<int> levels;
    for (int i = 0; i < frames; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &strs, &sc);
        for (int j = 0; j + 5 < cc;) {
            const int sz = cmd_size(cmds, j);
            if (j + sz > cc) break;
            if (cmds[j] == 7) levels.insert(cmds[j + 5]); // extrusionLevel
            j += sz;
        }
    }
    return levels;
}

} // namespace

TEST_CASE("cube/first frame emits cube records, not just clear-screen") {
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
    // rect/line were emitted. Cube nodes now arrive as opcode-7 records.
    REQUIRE(cc > 6);
    // Opcode 7 spans two 6-int slots, so the buffer stays uniformly 6-int
    // divisible — a parser advancing 6 ints per slot never desyncs (graceful
    // degradation for frontends that don't know opcode 7).
    CHECK(cc % 6 == 0);
    CHECK(count_opcode(ctx, 3, 7) > 0); // cube records

    dasher_set_long_parameter(ctx, lp_shape, 1); // restore
}

TEST_CASE("cube/forward driving emits cubes and labels") {
    ScopedContext ctx(800, 600);
    const int lp_shape = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(lp_shape > 0);
    dasher_set_long_parameter(ctx, lp_shape, LP_SHAPE_TYPE_CUBE);

    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);

    const int cubes = count_opcode(ctx, 30, 7);  // opcode-7 cube records
    const int labels = count_opcode(ctx, 30, 5); // opcode-5 text labels

    dasher_mouse_up(ctx);

    CHECK(cubes > 50); // one cube record per visible node
    CHECK(labels > 5); // Draw3DLabel -> opcode 5

    dasher_set_long_parameter(ctx, lp_shape, 1); // restore
}

TEST_CASE("cube/extrusion depth survives the command buffer") {
    // Issue #49: DrawCube used to discard CubeDepthLevel. Now each opcode-7
    // record carries the node's extrusionLevel, which must vary across the
    // node tree (root vs. children) — proving the depth data reaches the
    // frontend intact rather than being flattened.
    ScopedContext ctx(800, 600);
    const int lp_shape = dasher_find_parameter_key("LP_SHAPE_TYPE");
    REQUIRE(lp_shape > 0);
    dasher_set_long_parameter(ctx, lp_shape, LP_SHAPE_TYPE_CUBE);

    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);
    const std::set<int> levels = collect_extrusion_levels(ctx, 40);
    dasher_mouse_up(ctx);

    REQUIRE(!levels.empty());
    CHECK(levels.size() >= 2); // multiple depth levels => depth not discarded

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
