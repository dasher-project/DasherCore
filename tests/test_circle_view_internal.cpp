// test_circle_view_internal.cpp
//
// INTERNAL unit tests for the circle node-shape fixes in CDasherViewSquare,
// exercising the buggy code paths directly (no engine/CAPI involvement) so the
// reproduction is deterministic.
//
//   Bug 2 (deterministic here): DasherSpaceArc() -> CircleTo() used to recurse
//   without bound when an arc endpoint lay outside the circle (|y - cy| > r),
//   because sqrt(negative) -> NaN poisoned the convergence test. This drove the
//   game-mode brachistochrone (GameModule.cpp:235) into a stack overflow. The
//   degenerate args below are constructed so |cy - y_mid| > r for the recursive
//   midpoint, which is exactly the poisoning condition.
//
//   Bug 1 (smoke): IsSpaceAroundNode() used to DASHER_ASSERT(CoversCrosshair)
//   for every shape. That invariant is false for non-rectangular shapes, so in
//   circle mode the assert is a latent SIGABRT. We can't easily build a node
//   that both spans the visible region AND misses the crosshair on a default
//   screen, but we can at least confirm circle-mode IsSpaceAroundNode runs
//   without aborting across a sweep of bounds (the assert site is reached for
//   the root-sized span).
//
// Built via dasher_add_test_internal (links DasherCore directly).

#include "test_common.h"

#include "DasherCore/ColorPalette.h"
#include "DasherCore/DasherScreen.h"
#include "DasherCore/DasherViewSquare.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/SettingsStore.h"

#include <atomic>

using namespace Dasher;

namespace {

// Minimal CDasherScreen: records how many points it was asked to draw and
// otherwise does nothing. CircleTo's output lands in Polyline/Polygon.
class StubScreen : public CDasherScreen {
  public:
    StubScreen(screenint w, screenint h) : CDasherScreen(w, h) {}

    std::atomic<int> polyline_points{0};
    std::atomic<int> polygon_points{0};

    std::pair<screenint, screenint> TextSize(Label*, unsigned int) override { return {1, 1}; }
    void DrawString(Label*, screenint, screenint, unsigned int, const ColorPalette::Color&) override {}
    void DrawRectangle(screenint, screenint, screenint, screenint, const ColorPalette::Color&,
                       const ColorPalette::Color&, int) override {}
    void DrawCircle(screenint, screenint, screenint, const ColorPalette::Color&, const ColorPalette::Color&,
                    int) override {}
    void Polyline(point* pts, int n, int, const ColorPalette::Color&) override {
        if (pts && n > 0) polyline_points += n;
    }
    void Polygon(point* pts, int n, const ColorPalette::Color&, const ColorPalette::Color&, int) override {
        if (pts && n > 0) polygon_points += n;
    }
    void Display() override {}
    bool IsPointVisible(screenint, screenint) override { return true; }
};

struct ViewFixture {
    CSettingsStore store;
    StubScreen screen{800, 600};
    std::unique_ptr<CDasherViewSquare> view;

    explicit ViewFixture(Options::ScreenOrientations orient = Options::LeftToRight) {
        // Populate the default parameter table (LP_SHAPE_TYPE, BP_NONLINEAR_Y,
        // LP_GEOMETRY, ...). CSettingsStore::LoadPersistent is protected, but
        // AddParameters is public and takes the same defaults table.
        store.AddParameters(Settings::parameter_defaults);
        store.SetLongParameter(LP_SHAPE_TYPE, Options::CIRCLE);
        view = std::make_unique<CDasherViewSquare>(&store, &screen, orient);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Bug 2: a degenerate arc (endpoint outside the circle) must terminate.
// Without the fix this is UB (NaN -> integer cast) and, depending on the
// platform/orientation, recurses until the stack overflows (SIGSEGV).
// ---------------------------------------------------------------------------

TEST_CASE("circle/DasherSpaceArc degenerate arc does not recurse forever") {
    for (auto orient : {Options::LeftToRight, Options::TopToBottom}) {
        ViewFixture f(orient);

        // cy = ORIGIN_Y, r small. Endpoint y2 is far outside [cy-r, cy+r], so
        // the recursive midpoint satisfies |cy - y_mid| > r => sqrt(negative)
        // => NaN. r is chosen <= ORIGIN_X so DasherSpaceArc skips its
        // bisect-at-apex branch and feeds the degenerate endpoints to CircleTo.
        const myint cy = CDasherModel::ORIGIN_Y;
        const myint r = 100; // well inside ORIGIN_X (2048)
        const myint y1 = CDasherModel::ORIGIN_Y;
        const myint y2 = CDasherModel::ORIGIN_Y + 100000; // far outside the circle

        const ColorPalette::Color guideColor{255, 255, 255, 255};

        // If CircleTo recurses without bound this never returns.
        f.view->DasherSpaceArc(cy, r, CDasherModel::ORIGIN_X, y1, 0, y2, guideColor, 1);

        // It must actually have produced geometry.
        CHECK(f.screen.polyline_points.load() > 0);
    }
}

// A second degenerate configuration: arc spanning across cy with r > ORIGIN_X
// (exercises DasherSpaceArc's apex-bisection branch too).
TEST_CASE("circle/DasherSpaceArc large-radius cross-centre arc terminates") {
    ViewFixture f;

    const myint cy = CDasherModel::ORIGIN_Y - 5000;
    const myint r = CDasherModel::ORIGIN_X + 1000; // > ORIGIN_X => apex branch taken
    // y1 and y2 on opposite sides of cy => ((y1<cy)^(y2<cy)) is true.
    const myint y1 = cy - 40000; // outside circle: |y1 - cy| = 40000 > r
    const myint y2 = cy + 40000;

    const ColorPalette::Color guideColor{255, 0, 0, 255};

    f.view->DasherSpaceArc(cy, r, CDasherModel::ORIGIN_X, y1, 0, y2, guideColor, 2);
    CHECK(f.screen.polyline_points.load() > 0);
}

// ---------------------------------------------------------------------------
// Bug 1 smoke: circle-mode IsSpaceAroundNode must not abort (debug builds).
// A root-sized span reaches the (now guarded) assert site.
// ---------------------------------------------------------------------------

TEST_CASE("circle/IsSpaceAroundNode runs without aborting") {
    ViewFixture f;

    // Root-sized node: spans the whole visible region, so the early-return in
    // IsSpaceAroundNode is NOT taken and the (formerly unconditional, now
    // shape-guarded) assert site is reached.
    (void)f.view->IsSpaceAroundNode(0, CDasherModel::MAX_Y); // must return, not abort

    // A range of non-trivial spans should also evaluate cleanly.
    for (myint span : {CDasherModel::MAX_Y, CDasherModel::MAX_Y / 2, CDasherModel::MAX_Y / 4}) {
        const myint y1 = CDasherModel::ORIGIN_Y - span / 2;
        const myint y2 = CDasherModel::ORIGIN_Y + span / 2;
        (void)f.view->IsSpaceAroundNode(y1, y2); // must return, not abort
    }
}
