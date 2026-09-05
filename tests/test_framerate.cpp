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
    // Tight band: the estimate re-converges to the same cadence, ±20%.
    ASSERT(after > before * 4 / 5);
    ASSERT(after < before * 6 / 5);
}

TEST(framerate_sustained_slow_cadence_is_still_measured) {
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    // A frontend genuinely rendering at ~3.3fps (300ms/frame): the first
    // few long gaps are treated as suspensions, but the cadence must then
    // be measured — otherwise Steps() stays tuned for 60fps and text entry
    // stalls at the true bit-rate's fraction (review finding on the guard).
    f.feed(200, 5000, 300);
    const long after = f.lpFramerate();
    printf("  sustained 3.3fps: before=%ld after=%ld\n", before, after);
    ASSERT(after < before / 2); // adapted well down toward the real rate
}

TEST(framerate_bursty_throttling_is_eventually_measured) {
    // Review probe: <=3 long gaps + 1 normal frame, repeating (~4.4fps
    // effective — the classic timer-coalescing / IME-throttling shape).
    // A counter re-armed by any normal frame froze the estimate at 62fps
    // forever under this pattern; the time-weighted budget must let the
    // cadence through.
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    unsigned long t = 5000;
    for (int burst = 0; burst < 60; burst++) {
        for (int i = 0; i < 3; i++) {
            f.feed(1, t, 1);
            t += 300; // long gap after this frame
        }
        f.feed(1, t, 1);
        t += 16; // the lone normal frame
    }
    const long after = f.lpFramerate();
    printf("  bursty 3x300+16ms pattern: before=%ld after=%ld\n", before, after);
    ASSERT(after < before / 2); // the ~4.4fps reality got measured
}

TEST(framerate_intentional_pause_survives_reset_framerate) {
    // Reset_framerate (engine pause) sets m_iLastFrameTime, so a long
    // intentional pause is not a suspension and — critically — is not
    // folded into the fresh window either. Pins the Reset_framerate
    // interaction the review asked about.
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    f.framerate->Reset_framerate(60000); // 56s pause while unpaused engine
    f.feed(60, 60000, 16);
    const long after = f.lpFramerate();
    printf("  after 56s pause via Reset_framerate: before=%ld after=%ld\n", before, after);
    ASSERT(after > before * 4 / 5);
    ASSERT(after < before * 6 / 5);
}

TEST(framerate_251ms_boundary_is_measured) {
    // 251ms > 250ms threshold: guarded at first, but the budget must let
    // the true ~4fps cadence through (boundary probe from the review).
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    f.feed(120, 5000, 251);
    const long after = f.lpFramerate();
    printf("  steady 251ms cadence: before=%ld after=%ld\n", before, after);
    ASSERT(after < before / 2); // ~4fps measured, not frozen at 62fps
}

TEST(framerate_scheduler_jitter_is_not_a_suspension) {
    FramerateFixture f;
    f.feed(200, 1000, 16);
    const long before = f.lpFramerate();

    // Genuinely slow rendering (216ms gaps between 16ms frame pairs —
    // ~8.6fps effective, every gap under the 250ms threshold) must still
    // be measured: this is a slow cadence, not a suspension.
    for (int i = 0; i < 60; i++) {
        f.feed(2, 10000 + i * 232, 16);
    }
    const long after = f.lpFramerate();
    printf("  genuine slowdown measured: before=%ld after=%ld\n", before, after);
    ASSERT(after < before * 3 / 4); // the estimate did track the slowdown
}
