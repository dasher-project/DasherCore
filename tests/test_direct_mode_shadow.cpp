// Sweep: shallow reversals of varying length (delete ~1 word) followed by
// forward zooms — hunting for the "messes up the previous word" divergence.
#include "test_common.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct ShadowBuffer {
    std::string buf;
    std::vector<std::string> log;

    void onEvent(int type, const char* text) {
        char header[64];
        snprintf(header, sizeof(header), "%d:", type);
        log.push_back(std::string(header) + (text ? text : ""));
        if (type == 0 && text) {
            buf += text;
        } else if (type == 1 && text) {
            int cps = 0;
            for (const unsigned char* p = (const unsigned char*)text; *p; ++p)
                if ((*p & 0xC0) != 0x80) cps++;
            for (int i = 0; i < cps && !buf.empty(); i++) {
                buf.pop_back();
                while (!buf.empty() && ((unsigned char)buf.back() & 0xC0) == 0x80) buf.pop_back();
            }
        } else if (type == 2) {
            buf.clear();
        }
    }
};

ShadowBuffer g_shadow;

void steer(dasher_ctx* ctx, float x, float y, int frames, unsigned long& t) {
    dasher_mouse_move(ctx, x, y);
    run_frames(ctx, frames, t);
    t += frames * 16;
}

bool check(dasher_ctx* ctx, const char* stage, int& divergences) {
    const char* engine = dasher_get_output_text(ctx);
    std::string eng = engine ? engine : "";
    if (eng != g_shadow.buf) {
        divergences++;
        printf("  DIVERGENCE at %s:\n    engine: '%s'\n    shadow: '%s'\n", stage, eng.c_str(),
               g_shadow.buf.c_str());
        const size_t n = g_shadow.log.size();
        for (size_t i = n >= 16 ? n - 16 : 0; i < n; i++)
            printf("    event '%s'\n", g_shadow.log[i].c_str());
        return false;
    }
    return true;
}

} // namespace

TEST(direct_mode_sweep_reverse_lengths) {
    int scenarios = 0, failures = 0;

    // Vary the reverse duration: shallow (a word) through deep, with and
    // without mouse-up pauses between the phases.
    const int reverseFrames[] = {30, 40, 50, 60, 80, 100, 120, 150, 200, 250};
    for (int rev : reverseFrames) {
        for (int pause = 0; pause <= 1; pause++) {
            dasher_ctx* ctx = create_isolated_context();
            ASSERT(ctx != nullptr);
            g_shadow.buf.clear();
            g_shadow.log.clear();
            dasher_set_output_callback(
                ctx,
                [](int e, const char* t, void*) { g_shadow.onEvent(e, t); },
                nullptr);
            dasher_set_speed_percent(ctx, 300);
            dasher_set_screen_size(ctx, 800, 600);

            unsigned long t = 1000;
            char stage[128];
            scenarios++;

            dasher_mouse_move(ctx, 700.0f, 300.0f);
            dasher_mouse_down(ctx);
            steer(ctx, 700.0f, 285.0f, 220, t);
            steer(ctx, 700.0f, 300.0f, 120, t);
            steer(ctx, 700.0f, 288.0f, 180, t);
            snprintf(stage, sizeof(stage), "rev=%d pause=%d after-forward1", rev, pause);
            if (!check(ctx, stage, failures)) {
                dasher_destroy(ctx);
                continue;
            }

            if (pause) dasher_mouse_up(ctx);
            steer(ctx, 60.0f, 300.0f, rev, t);
            if (pause) dasher_mouse_down(ctx);
            snprintf(stage, sizeof(stage), "rev=%d pause=%d after-reverse", rev, pause);
            if (!check(ctx, stage, failures)) {
                dasher_destroy(ctx);
                continue;
            }

            steer(ctx, 700.0f, 285.0f, 160, t);
            steer(ctx, 700.0f, 300.0f, 120, t);
            snprintf(stage, sizeof(stage), "rev=%d pause=%d after-forward2", rev, pause);
            check(ctx, stage, failures);

            dasher_mouse_up(ctx);
            dasher_destroy(ctx);
        }
    }

    printf("  scenarios: %d, failures: %d\n", scenarios, failures);
    ASSERT_EQ(failures, 0);
}
