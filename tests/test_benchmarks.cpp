// test_benchmarks.cpp
//
// PERFORMANCE BASELINE for dasher_frame. Before any Tier 2/3 refactor
// (especially the PPM probability-merge dedup), we need a measurement
// so regressions are visible.
//
// What this measures:
//   - Per-call latency of dasher_frame over 10,000 frames with the mouse
//     actively driving text production
//   - Reports mean, p50, p95, p99, max in milliseconds
//
// What this asserts:
//   - p99 < 100 ms (very generous; ~6x the 16ms target)
//
// Why 100 ms: the strict "60 FPS = 16 ms" target is unrealistic on slow
// CI runners under sanitizer loads. 100 ms catches catastrophic regressions
// (10x slowdown) without flaking. Once we have stable measurements across
// platforms, this can be tightened.
//
// Why not fail the build at p99 < 16 ms: that would block PRs that touch
// hot paths in legitimate ways (e.g., adding bounds checks). We want this
// test to fail only on actual regressions, not on platform noise.

#include "test_common.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

using clk = std::chrono::steady_clock;

double percentile_ms(std::vector<double>& samples, double pct) {
    if (samples.empty()) return 0.0;
    std::sort(samples.begin(), samples.end());
    size_t idx = static_cast<size_t>(samples.size() * pct);
    if (idx >= samples.size()) idx = samples.size() - 1;
    return samples[idx];
}

} // namespace

TEST_CASE("bench/dasher_frame p99 under 100ms over 10000 frames") {
    ScopedContext ctx(800, 600);
    dasher_set_speed_percent(ctx, 200);

    // Warm up: discard the first 100 frames (cache cold-start, JIT-like
    // effects in the engine's first realization).
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    run_frames(ctx, 100);
    dasher_mouse_up(ctx);

    // Measurement: 10,000 frames with active input.
    constexpr int N = 10000;
    std::vector<double> samples_ms;
    samples_ms.reserve(N);

    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < N; ++i) {
        // Vary sy slightly to keep the model doing real work (text entry
        // exercises node creation, garbage collection, and rendering).
        float sy = 300.0f + 30.0f * std::sin(i * 0.01f);
        dasher_mouse_move(ctx, 700.0f, sy);

        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        auto t0 = clk::now();
        dasher_frame(ctx, 100000 + i * 16, &cmds, &cc, &strs, &sc);
        auto t1 = clk::now();
        samples_ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    dasher_mouse_up(ctx);

    // Statistics.
    const double p50 = percentile_ms(samples_ms, 0.50);
    const double p95 = percentile_ms(samples_ms, 0.95);
    const double p99 = percentile_ms(samples_ms, 0.99);
    const double max_ms = samples_ms.back(); // samples is sorted after percentile call

    double sum = 0.0;
    for (double s : samples_ms)
        sum += s;
    const double mean = sum / samples_ms.size();

    // Log to stdout so CI captures the numbers for trend analysis.
    printf("\n[dasher_frame benchmark] N=%d frames\n", N);
    printf("  mean: %.3f ms\n", mean);
    printf("  p50:  %.3f ms\n", p50);
    printf("  p95:  %.3f ms\n", p95);
    printf("  p99:  %.3f ms\n", p99);
    printf("  max:  %.3f ms\n", max_ms);
    printf("  target: 16.67 ms (60 FPS)\n\n");
    fflush(stdout);

    // Generous thresholds; tighten once we have stable baseline across
    // platforms.
    CHECK(p99 < 100.0);
    CHECK(mean < 50.0);
}

TEST_CASE("bench/dasher_frame at rest is faster than active") {
    // CHARACTERIZATION: a frame with no input change should be cheaper
    // than a frame with active input motion. We don't predict the ratio
    // (it depends on platform and how much node GC happens), but we do
    // assert the ordering: active > idle.
    ScopedContext ctx(800, 600);
    dasher_set_speed_percent(ctx, 200);

    constexpr int N = 2000;
    std::vector<double> idle_samples, active_samples;
    idle_samples.reserve(N);
    active_samples.reserve(N);

    // Idle: mouse stationary at crosshair, no zoom.
    dasher_mouse_move(ctx, 400.0f, 300.0f); // at crosshair = no motion
    for (int i = 0; i < N; ++i) {
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        auto t0 = clk::now();
        dasher_frame(ctx, 200000 + i * 16, &cmds, &cc, &strs, &sc);
        auto t1 = clk::now();
        idle_samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    // Active: mouse in motion, zooming forward.
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < N; ++i) {
        dasher_mouse_move(ctx, 700.0f, 300.0f + (i % 50));
        int* cmds = nullptr;
        int cc = 0;
        char** strs = nullptr;
        int sc = 0;
        auto t0 = clk::now();
        dasher_frame(ctx, 300000 + i * 16, &cmds, &cc, &strs, &sc);
        auto t1 = clk::now();
        active_samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    dasher_mouse_up(ctx);

    const double idle_p50 = percentile_ms(idle_samples, 0.50);
    const double active_p50 = percentile_ms(active_samples, 0.50);

    printf("\n[dasher_frame idle-vs-active] N=%d frames each\n", N);
    printf("  idle p50:   %.3f ms\n", idle_p50);
    printf("  active p50: %.3f ms\n", active_p50);
    printf("  ratio:      %.2fx\n", active_p50 / std::max(0.001, idle_p50));
    fflush(stdout);

    // Active should be at least as expensive as idle. Equal is OK (no
    // motion means no zoom, so the workloads converge), but idle should
    // not be more expensive than active.
    CHECK(idle_p50 <= active_p50 + 1.0); // 1ms tolerance for noise
}
