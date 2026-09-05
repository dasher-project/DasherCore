// Frame-rate estimation regression tests.
//
// Dasher-Android #35: in the floating-keyboard IME the process gets briefly
// starved (overlay IMEs are low priority); on resume the wall-clock gap was
// folded into CFrameRate's sampling window, collapsing LP_FRAMERATE,
// shrinking Steps(), and making every post-stall frame zoom a large
// fraction at once — "lags, then the letters jump". The suspension guard
// treats a >250ms inter-frame gap as a pause: the partial window is dropped
// and the pre-stall estimate survives.
#include "test_common.h"

#include "DasherCore/FrameRate.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/SettingsStore.h"

#include <memory>

using namespace Dasher;

namespace {

struct FramerateFixture {
    CSettingsStore store;
    std::unique_ptr<CFrameRate> framerate;

    FramerateFixture() {
        // A bare CSettingsStore starts with an empty parameter table, and
        // CFrameRate's ctor reads LP_X_LIMIT_SPEED from it — populate from
        // the manifest FIRST (as the engine does on startup), then build.
        store.AddParameters(Settings::parameter_defaults);
        framerate = std::make_unique<CFrameRate>(&store);
    }

    long lpFramerate() const { return store.GetLongParameter(LP_FRAMERATE); }

    // Feed frames at the given cadence starting at Time.
    void feed(int count, unsigned long startMs, unsigned long stepMs) {
        for (int i = 0; i < count; i++)
            framerate->RecordFrame(startMs + i * stepMs);
    }
};

} // namespace

TEST(framerate_steady_60fps_estimates_around_60) {
    FramerateFixture f;
    // LP_FRAMERATE default is 2500 (25fps in hundredths); drive it to ~60.
    f.feed(200, 1000, 16);
    printf("  LP_FRAMERATE after 200 @16ms: %ld\n", f.lpFramerate());
    ASSERT(f.lpFramerate() > 4000); // ≈40fps+ — the estimate tracked reality
}

TEST(framerate_stall_does_not_collapse_the_estimate) {
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    // 600ms suspension (float-IME throttling / GC), then frames resume.
    // Last pre-stall frame is at 1000 + 199*16 = 4184ms; resume at 5000ms.
    f.feed(60, 5000, 16);
    const long after = f.lpFramerate();
    printf("  LP_FRAMERATE before stall: %ld, after stall+resume: %ld\n", before, after);

    // Without the guard the estimate collapsed toward ~3fps (a frame pair
    // spanning the 600ms gap), shrinking Steps() and making every
    // post-stall frame a mega-zoom. With it, the estimate survives.
    ASSERT(after >= before * 9 / 10);
}

TEST(framerate_stall_recovers_without_overshoot) {
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    // A stall, then a long steady run at the same real cadence: the estimate
    // must stay near the pre-stall value (no collapse, no runaway growth).
    f.feed(400, 5000, 16);
    const long after = f.lpFramerate();
    printf("  steady after stall: before=%ld after=%ld\n", before, after);
    ASSERT(after > before / 2);
    ASSERT(after < before * 2);
}

TEST(framerate_scheduler_jitter_is_not_a_suspension) {
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    // Normal jitter — occasional 100ms hiccup — must still be measured
    // (this is genuinely slow rendering, not a suspension).
    for (int i = 0; i < 60; i++) {
        f.feed(2, 10000 + i * 232, 16);
        // (2 frames per 232ms window ≈ 8.6fps average)
    }
    const long after = f.lpFramerate();
    printf("  genuine slowdown measured: before=%ld after=%ld\n", before, after);
    ASSERT(after < before * 3 / 4); // the estimate did track the slowdown
}
