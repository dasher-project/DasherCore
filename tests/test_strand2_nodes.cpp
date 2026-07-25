// test_strand2_nodes.cpp
//
// Tests for the Strand 2 custom-rendering API (RFC 0013):
//   dasher_set_visible_nodes_enabled / dasher_get_visible_nodes / dasher_get_viewport
//
// Key invariant: the node set captured for Strand 2 must match exactly what the
// Strand 1 command buffer draws for the same frame (Strand 1/Strand 2 parity),
// so a frontend can mix strands. We verify that each Strand 2 node's screen
// bounds correspond to an opcode-4 filled rectangle from dasher_frame().

#include "test_common.h"

#include <algorithm>
#include <set>
#include <tuple>
#include <vector>

namespace {

struct Rect {
    int x1, y1, x2, y2;
    bool operator<(const Rect& o) const { return std::tie(x1, y1, x2, y2) < std::tie(o.x1, o.y1, o.x2, o.y2); }
    bool operator==(const Rect& o) const { return std::tie(x1, y1, x2, y2) == std::tie(o.x1, o.y1, o.x2, o.y2); }
};

// Initialise struct_size on every element of a node buffer (ABI version marker).
void init_node_info(dasher_node_info* nodes, int n) {
    for (int i = 0; i < n; ++i)
        nodes[i].struct_size = sizeof(dasher_node_info);
}

} // namespace

TEST_CASE("strand2/disabled by default; enabling works") {
    ScopedContext ctx(800, 600);
    dasher_node_info nodes[8];
    init_node_info(nodes, 8);
    char** strs = nullptr;
    int sc = 0;

    // Before enabling, capture is off -> dasher_get_visible_nodes returns -1.
    run_frames(ctx, 3);
    CHECK(dasher_get_visible_nodes(ctx, nodes, 8, &strs, &sc) == -1);

    // Enable, advance a frame (capture happens during dasher_frame), then query.
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);
    run_frames(ctx, 1);
    const int n = dasher_get_visible_nodes(ctx, nodes, 8, &strs, &sc);
    CHECK(n > 0);
}

TEST_CASE("strand2/depth-first preorder invariant on depths") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);

    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);

    std::vector<dasher_node_info> nodes(64);
    init_node_info(nodes.data(), 64);
    const int n = dasher_get_visible_nodes(ctx, nodes.data(), 64, &strs, &sc);
    dasher_mouse_up(ctx);
    REQUIRE(n > 0);
    nodes.resize(std::min(n, 64));

    // First node is the rendered root (depth 0).
    CHECK(nodes[0].depth == 0);

    // DFS preorder invariant: a node's depth can be at most one greater than
    // the immediately preceding node's depth (you can only step one level
    // deeper per recursion; backtracking only goes shallower).
    for (size_t i = 1; i < nodes.size(); ++i) {
        CHECK(nodes[i].depth <= nodes[i - 1].depth + 1);
    }
}

TEST_CASE("strand2/Strand 1 vs Strand 2 parity (bounds match opcode-4 rects)") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    // One frame: Strand 1 (commands) and Strand 2 (captured nodes) are produced
    // by the SAME Render() pass, so they must agree.
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);
    dasher_mouse_up(ctx);

    // Strand 1: collect opcode-4 (filled rect) bounds.
    std::multiset<Rect> rects;
    for (int j = 0; j + 5 < cc; j += 6) {
        if (cmds[j] == 4) rects.insert({cmds[j + 1], cmds[j + 2], cmds[j + 3], cmds[j + 4]});
    }
    REQUIRE(!rects.empty());

    // Strand 2: captured nodes.
    std::vector<dasher_node_info> nodes(256);
    init_node_info(nodes.data(), 256);
    const int n = dasher_get_visible_nodes(ctx, nodes.data(), 256, &strs, &sc);
    REQUIRE(n > 0);
    nodes.resize(std::min(n, 256));

    // Every Strand 2 node's bounds must be present in the Strand 1 rect set.
    // (Strand 1 also emits non-node rects — background/margin blanking — so its
    //  set is a superset of the node rects.)
    int matched = 0;
    for (const auto& nd : nodes) {
        Rect r{nd.screen_x1, nd.screen_y1, nd.screen_x2, nd.screen_y2};
        auto it = rects.find(r);
        if (it != rects.end()) {
            ++matched;
            rects.erase(it); // account for multiplicity (distinct nodes)
        }
    }
    CHECK(matched == (int)nodes.size());
}

TEST_CASE("strand2/labels and symbols are populated") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);

    // The default alphabet's top level is group nodes ("Lower case Latin
    // letters", ...); symbol nodes live inside them. Drive forward enough to
    // zoom into a group and expose its symbol children.
    dasher_set_speed_percent(ctx, 250);
    dasher_mouse_move(ctx, 720.0f, 300.0f);
    dasher_mouse_down(ctx);
    int* cmds = nullptr;
    int cc = 0;
    char** s2 = nullptr;
    int s2c = 0;
    for (int i = 0; i < 25; ++i)
        dasher_frame(ctx, 1000 + i * 16, &cmds, &cc, &s2, &s2c);
    dasher_mouse_up(ctx);

    std::vector<dasher_node_info> nodes(128);
    init_node_info(nodes.data(), 128);
    char** strs = nullptr;
    int sc = 0;
    const int n = dasher_get_visible_nodes(ctx, nodes.data(), 128, &strs, &sc);
    REQUIRE(n > 0);
    nodes.resize(std::min(n, 128));

    // At least one node carries a non-empty label (groups and symbols both have
    // labels); label indices index into the strings array.
    int with_label = 0;
    for (const auto& nd : nodes) {
        if (nd.label_index >= 0) {
            CHECK(nd.label_index < sc);
            CHECK(strs[nd.label_index][0] != '\0');
            ++with_label;
        }
    }
    CHECK(with_label > 0);

    // Symbol field is sane for every node: -1 for group/control nodes, or a
    // valid alphabet index for symbol nodes. Driving deep exposes >=1 symbol.
    const int sym_count = dasher_get_alphabet_symbol_count(ctx);
    int with_symbol = 0;
    for (const auto& nd : nodes) {
        if (nd.symbol >= 0) {
            CHECK(nd.symbol < sym_count);
            ++with_symbol;
        } else {
            CHECK(nd.symbol == -1);
        }
    }
    CHECK(with_symbol > 0);
}

TEST_CASE("strand2/viewport matches engine constants and screen size") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);
    run_frames(ctx, 1);

    dasher_viewport vp;
    vp.struct_size = sizeof(dasher_viewport);
    REQUIRE(dasher_get_viewport(ctx, &vp) == 0);

    CHECK(vp.screen_width == 800);
    CHECK(vp.screen_height == 600);
    // Crosshair is fixed at the Dasher origin (2048, 2048).
    CHECK(vp.crosshair_x == 2048);
    CHECK(vp.crosshair_y == 2048);
    // Visible Y range straddles the crosshair.
    CHECK(vp.visible_min_y < 2048);
    CHECK(vp.visible_max_y > 2048);
}

TEST_CASE("strand2/struct_size ABI guard rejects undersized caller struct") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);
    run_frames(ctx, 1);

    dasher_node_info nodes[2];
    init_node_info(nodes, 2);
    char** strs = nullptr;
    int sc = 0;

    // Caller claims a struct smaller than the engine's -> engine refuses rather
    // than overflow the caller's allocation.
    nodes[0].struct_size = 4;
    CHECK(dasher_get_visible_nodes(ctx, nodes, 2, &strs, &sc) == -1);
}

TEST_CASE("strand2/bounds within screen and monotone") {
    ScopedContext ctx(800, 600);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);
    run_frames(ctx, 1);

    std::vector<dasher_node_info> nodes(64);
    init_node_info(nodes.data(), 64);
    char** strs = nullptr;
    int sc = 0;
    const int n = dasher_get_visible_nodes(ctx, nodes.data(), 64, &strs, &sc);
    REQUIRE(n > 0);
    nodes.resize(std::min(n, 64));

    for (const auto& nd : nodes) {
        CHECK(nd.screen_x1 <= nd.screen_x2);
        CHECK(nd.screen_y1 <= nd.screen_y2);
        // Bounds may extend slightly off-canvas (clipped to the visible region in
        // Dasher space, not the canvas), but the top-left should be sane.
        CHECK(nd.screen_x2 >= 0);
        CHECK(nd.screen_y2 >= 0);
    }
}
