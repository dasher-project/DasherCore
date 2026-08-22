// Tests for the frontend text-measurement callback (issue #56).
//
// The engine's label layout (the anti-overlap "shunting" in
// CDasherViewSquare::DoDelayedText) consumes CDasherScreen::TextSize. The
// command-buffer screen's built-in estimate (codepoints × fontSize/2)
// compounds down the label chain, which users see as jumbled text at deep
// zoom; the callback lets frontends supply real font metrics instead.
//
// Covered here through the public C API only:
//   - the callback is actually consulted during dasher_frame
//   - per-label/per-size caching keeps steady-state frames callback-free
//   - dasher_text_metrics_changed invalidates the cache
//   - registering before dasher_set_screen_size still takes effect
//   - a failing callback falls back to the estimate without caching it

#include "test_common.h"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace {

struct MeasureState {
    int calls = 0;
    int fail_calls = 0; // when >0, the next that many calls fail
    // Cache contract under test: each (text, font size) pair must be measured
    // at most once per metrics generation. The view may discover NEW labels
    // as the node tree grows, so raw call counts aren't stable across frames.
    std::map<std::pair<std::string, int>, int> calls_per_pair;
    std::function<int(const std::string&, int)> width_of;

    int max_pair_calls() const {
        int m = 0;
        for (const auto& entry : calls_per_pair) {
            m = std::max(m, entry.second);
        }
        return m;
    }
};

// Shared measurement callback: consults MeasureState, honours fail_calls.
int measure_trampoline(const char* text, int font_size, int* w, int* h, void* ud) {
    auto* s = static_cast<MeasureState*>(ud);
    if (!text || !w || !h) return 1;
    if (s->fail_calls > 0) {
        s->fail_calls--;
        return 1; // frontend could not measure
    }
    s->calls++;
    s->calls_per_pair[{text, font_size}]++;
    *w = s->width_of(text, font_size);
    *h = font_size;
    return 0;
}

} // namespace

TEST_CASE("text size callback is consulted during frames and cached per label/size") {
    dasher_ctx* ctx = create_isolated_context();
    REQUIRE(ctx != nullptr);

    MeasureState st;
    st.width_of = [](const std::string& text, int font_size) {
        // Deterministic "real" metrics: 3/4 em per glyph, wider than the
        // engine's size/2 estimate — exactly the Segoe-UI-like case from
        // the jumbled-text reports.
        return static_cast<int>(text.size()) * font_size * 3 / 4;
    };

    // Wire the callback BEFORE the screen exists — frontends register
    // callbacks before starting the engine; dasher_set_screen_size must
    // forward it.
    dasher_set_text_size_callback(ctx, measure_trampoline, &st);
    dasher_set_screen_size(ctx, 800, 600);
    run_frames(ctx, 5);
    REQUIRE(st.calls > 0);

    // Steady state: a stable label must never be re-measured. New nodes may
    // appear as the tree grows, so the assertion is per (text,size) pair —
    // each measured at most once per generation.
    run_frames(ctx, 10);
    CHECK(st.max_pair_calls() == 1);

    // Metrics invalidation (canvas font changed) → labels re-measured.
    dasher_text_metrics_changed(ctx);
    run_frames(ctx, 1);
    CHECK(st.calls > 0);
    const int after_invalidation = st.max_pair_calls();
    run_frames(ctx, 5);
    CHECK(st.max_pair_calls() == after_invalidation); // and cached again

    dasher_destroy(ctx);
}

TEST_CASE("failing measurement falls back to the estimate and retries later") {
    dasher_ctx* ctx = create_isolated_context();
    REQUIRE(ctx != nullptr);

    MeasureState st;
    st.width_of = [](const std::string&, int font_size) { return font_size; };

    dasher_set_text_size_callback(ctx, measure_trampoline, &st);
    dasher_set_screen_size(ctx, 800, 600);

    st.fail_calls = 1000; // every measurement fails this phase
    run_frames(ctx, 3);
    CHECK(st.calls == 0); // engine survived on the estimate

    st.fail_calls = 0; // frontend recovers (e.g. font system ready)
    run_frames(ctx, 1);
    CHECK(st.calls > 0); // failures were not cached as estimates

    dasher_destroy(ctx);
}

TEST_CASE("null callback keeps the estimate path (no crash, frames render)") {
    dasher_ctx* ctx = create_isolated_context();
    REQUIRE(ctx != nullptr);

    dasher_set_text_size_callback(ctx, nullptr, nullptr);
    dasher_set_screen_size(ctx, 800, 600);
    // Just render: the regression contract is "no crash, text commands still
    // appear" — the estimate is unchanged behaviour for unwired frontends.
    run_frames(ctx, 5);

    int* cmds = nullptr;
    int cmd_count = 0;
    char** strs = nullptr;
    int str_count = 0;
    dasher_frame(ctx, 5000, &cmds, &cmd_count, &strs, &str_count);
    REQUIRE(cmds != nullptr);
    CHECK(cmd_count > 0);

    dasher_destroy(ctx);
}
