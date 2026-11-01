// test_paragraph_zoom.cpp
//
// Reproduction for the v0.1.35 field report (Heide): "the issue is back
// when you start a new paragraph for the FIRST time (zoom an enter button),
// the canvas goes crazy! ... Then the rest of the paragraph breaks seem to
// work fine." DasherCore #102 made the paragraph a hard prediction boundary
// by swapping the committed node's LM context for a fresh paragraph-seed
// context. A context swap re-weights every node under the crosshair; if the
// zoom dynamics overreact to that remap, the canvas runs away until the
// user re-seeds by typing a real Enter.
//
// Drives the engine exactly like the Windows frontend: seed a sentence,
// steer the crosshair into the paragraph node until it commits, then hold
// the pointer STILL (button held) and measure per-frame offset advancement.
// A runaway is the post-commit advancement rate exploding relative to the
// equivalent measurement on the SECOND paragraph break (which the field
// report says behaves normally).
#include "test_common.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

struct Box {
    int x1, y1, x2, y2;
    int cx() const { return (x1 + x2) / 2; }
    int cy() const { return (y1 + y2) / 2; }
};

// Finds the on-screen box of the paragraph node by locating its text label
// among the draw commands and taking the smallest enclosing RECT_FILL.
bool find_paragraph_box(int* cmds, int ncmds, char** strs, int nstrs, Box* out) {
    const int ops = ncmds / 6; // cmd_count is INT count; 6 ints per command
    for (int i = 0; i < ops; i++) {
        const int* c = cmds + i * 6;
        if (c[0] != 5 /* DASHER_CMD_TEXT */) continue;
        const int si = c[4];
        if (si < 0 || si >= nstrs) continue;
        const char* s = strs[si];
        if (!s) continue; // string table may carry NULL slots
        if (!(strchr(s, '\n') || strchr(s, '\r') || strstr(s, "\xc2\xb6") /* U+00B6 pilcrow */ ||
              strstr(s, "\xe2\x8f\x8e") /* U+23CE return */))
            continue;
        const int tx = c[1], ty = c[2];
        Box best{};
        bool have = false;
        for (int j = 0; j < ops; j++) {
            const int* r = cmds + j * 6;
            if (r[0] != 4 /* DASHER_CMD_RECT_FILL */) continue;
            if (r[1] <= tx && tx < r[3] && r[2] <= ty && ty < r[4]) {
                const long long area_best = 1LL * (best.x2 - best.x1) * (best.y2 - best.y1);
                const long long area_r = 1LL * (r[3] - r[1]) * (r[4] - r[2]);
                if (!have || area_r < area_best) {
                    best = {r[1], r[2], r[3], r[4]};
                    have = true;
                }
            }
        }
        if (have) {
            *out = best;
            return true;
        }
    }
    return false;
}

struct ZoomStats {
    int max_delta;
    double mean_delta;
    int chars_typed;
};

// Holds the pointer still (button held) and returns per-frame advancement
// stats: offset deltas AND characters entered (offset units are coarse;
// output length is the human-visible typing rate).
ZoomStats hold_pointer(dasher_ctx* ctx, int mx, int my, int frames) {
    int prev = dasher_get_offset(ctx);
    size_t prev_len = strlen(dasher_get_output_text(ctx));
    int maxd = 0;
    double sum = 0;
    int n = 0;
    int typed = 0;
    for (int i = 0; i < frames; i++) {
        dasher_mouse_move(ctx, (float)mx, (float)my);
        run_frames(ctx, 1, 1000000 + i * 16);
        const int off = dasher_get_offset(ctx);
        const size_t len = strlen(dasher_get_output_text(ctx));
        const int d = off > prev ? off - prev : prev - off;
        if (d > maxd) maxd = d;
        sum += d;
        n++;
        prev = off;
        typed += (int)(len > prev_len ? len - prev_len : 0);
        prev_len = len;
    }
    return {maxd, sum / (n ? n : 1), typed};
}

static int count_newlines(const char* s) {
    int n = 0;
    for (; s && *s; s++)
        if (*s == '\n' || *s == '\r') n++;
    return n;
}

// Steers into the paragraph node and waits for the wanted newline count.
// Reports the worst per-frame offset lurch during the zoom (a context-swap
// node remap shows up as one frame advancing vastly more than its
// neighbours).
bool zoom_into_paragraph(dasher_ctx* ctx, int want_newlines, int* worst_lurch) {
    Box box{};
    int prev = dasher_get_offset(ctx);
    int lurch = 0;
    for (int attempt = 0; attempt < 1200; attempt++) {
        int *cmds = nullptr, ncmds = 0;
        char** strs = nullptr;
        int nstrs = 0;
        dasher_frame(ctx, 500000 + attempt * 16, &cmds, &ncmds, &strs, &nstrs);
        const bool found = cmds && find_paragraph_box(cmds, ncmds, strs, nstrs, &box);
        if (found) {
            // aim at the box centre; x past the crosshair steers forward
            const int mx = box.cx() > 200 ? box.cx() : 260;
            dasher_mouse_move(ctx, (float)mx, (float)box.cy());
        } else {
            dasher_mouse_move(ctx, 400.0f, 300.0f); // neutral drift
        }
        const int off = dasher_get_offset(ctx);
        const int d = off > prev ? off - prev : prev - off;
        if (d > lurch) lurch = d;
        prev = off;
        const char* text = dasher_get_output_text(ctx);
        if (text && count_newlines(text) >= want_newlines) {
            if (worst_lurch) *worst_lurch = lurch;
            // newline representation diagnostic: what exactly does the engine
            // emit for a paragraph? (\n vs \r\n - frontends compare against
            // their text control's normalization)
            const size_t tl = strlen(text);
            printf("  [zoom] committed after %d frames, tail bytes:", attempt);
            for (size_t k = tl > 8 ? tl - 8 : 0; k < tl; k++)
                printf(" %02x", (unsigned char)text[k]);
            printf("\n");
            return true;
        }
    }
    return false;
}

} // namespace

TEST(paragraph_first_break_is_not_a_runaway) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_speed_percent(ctx, 500);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_seed_buffer(ctx, "Hello world this is a test", 26);
    dasher_mouse_move(ctx, 400.0f, 300.0f);
    dasher_mouse_down(ctx);

    // ---- paragraph #1: zoom the Enter node, then hold still ----
    int lurch1 = 0;
    REQUIRE(zoom_into_paragraph(ctx, 1, &lurch1));
    const ZoomStats first = hold_pointer(ctx, 700, 300, 90);
    printf("  first break:  lurch=%d hold max/frame=%d mean/frame=%.1f chars=%d\n", lurch1, first.max_delta,
           first.mean_delta, first.chars_typed);

    // ---- paragraph #2: more words, same zoom + hold ----
    dasher_seed_buffer(ctx, "Hello world this is a test.\r\nA second paragraph with some words", 61);
    int lurch2 = 0;
    REQUIRE(zoom_into_paragraph(ctx, 3, &lurch2)); // seed's CRLF is 2; the node adds the 3rd
    const ZoomStats second = hold_pointer(ctx, 700, 300, 90);
    printf("  second break: lurch=%d hold max/frame=%d mean/frame=%.1f chars=%d\n", lurch2, second.max_delta,
           second.mean_delta, second.chars_typed);

    dasher_mouse_up(ctx);
    dasher_destroy(ctx);

    // A runaway is an EXPLOSION, not a modest first-vs-second difference:
    // allow 4x plus slack. If #102's context swap shocks the zoom on the
    // first break only, first.max_delta lands far above second's.
    REQUIRE(first.max_delta <= 4 * second.max_delta + 64);
}
