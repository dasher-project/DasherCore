// test_paragraph_context.cpp
//
// A paragraph is a hard prediction boundary (AlphabetManager.cpp): whatever
// precedes the newline — period, space, bare word, comma, another newline —
// the context seen by the language model must be the canonical
// paragraph-start context (alphabet default context, trailing whitespace
// stripped, then the paragraph symbol). The bundled English corpus breaks
// paragraphs as ".\r\n" (94% of lines start with a capital after CRLF), so
// the canonical context gives the same strong sentence-start prediction the
// model produces after ". " (the uppercase group dominates).
//
// Before the fix, only [., ¶] matched training; every other predecessor
// chained THROUGH the paragraph and degraded to a mid-sentence mixture
// (uppercase group ~0.67, lowercase ~0.25) — the "Enter remembers the last
// punctuation / you have to zoom a capital" report.
//
// The assertions are mapping-free (no child-index-to-group knowledge
// needed): every paragraph context must yield EXACTLY the distribution of
// the known-good [.] + newline context, and that must differ from a
// mid-word context.
#include "test_common.h"

#include <cstdio>
#include <vector>

namespace {

std::vector<int> masses_after_seed(dasher_ctx* ctx, const char* seed) {
    REQUIRE(dasher_seed_buffer(ctx, seed, (int)strlen(seed)) == 0);
    int lb[64], hb[64];
    int n = dasher_get_probabilities(ctx, lb, hb, 64);
    REQUIRE(n > 0);
    std::vector<int> out;
    for (int i = 0; i < n; i++)
        out.push_back(hb[i] - lb[i]);
    return out;
}

} // namespace

TEST(paragraph_is_a_hard_prediction_boundary) {
    ScopedContext ctx(800, 600);

    // The known-good reference: sentence-ending period directly followed by
    // a newline (the corpus's dominant paragraph break).
    const std::vector<int> canonical = masses_after_seed(ctx, "n.\r\n");
    REQUIRE(canonical.size() > 1);

    // Every other predecessor of the newline must predict identically.
    const char* variants[] = {
        "n.\n",    // bare LF (what the alphabet's paragraph node outputs)
        "n. \r\n", // space before the newline (zoomed space then Enter)
        "n. \n",
        "nq\r\n", // bare word before the newline (no punctuation)
        "nq\n",
        "n,\r\n", // comma before the newline
        "n,\n",       "n, \r\n",
        "n.\r\n\r\n", // blank line between paragraphs
        "n.\n\n",
    };
    for (const char* v : variants)
        REQUIRE(masses_after_seed(ctx, v) == canonical);

    // ...and the paragraph context is NOT a mid-word context.
    const std::vector<int> midword = masses_after_seed(ctx, "nq");
    REQUIRE(midword != canonical);

    // The context also differs from a bare period (paragraphSpace dominates
    // there) and from a period+space (finer-order context) — i.e. the
    // canonical context is its own state, not an accidental alias.
    REQUIRE(masses_after_seed(ctx, "n.") != canonical);
}

// Live-path coverage: zoom the actual paragraph NODE (mouse steering at its
// screen rect, re-aimed every frame) from a mid-word-with-space state — the
// exact predecessor the old code mishandled — and require the post-Enter
// distribution to be the canonical paragraph-start one. This exercises the
// CreateSymbolNode boundary branch (greptile: "live newline paths lack
// coverage"); the adaptive SetFlag branch shares the same seed machinery.
TEST(paragraph_live_zoom_resets_context) {
    ScopedContext ctx(800, 600);
    dasher_set_speed_percent(ctx, 400);

    // Locate the paragraph symbol (its text is exactly "\n").
    char buf[64];
    int paraSym = -1;
    const int symCount = dasher_get_alphabet_symbol_count(ctx);
    for (int i = 0; i < symCount; i++)
        if (dasher_get_alphabet_symbol_text(ctx, i, buf, sizeof buf) == 0 && strcmp(buf, "\n") == 0) {
            paraSym = i;
            break;
        }
    REQUIRE(paraSym > 0);
    REQUIRE(dasher_set_visible_nodes_enabled(ctx, 1) == 0);

    // Reference distribution (seed path).
    const std::vector<int> canonical = masses_after_seed(ctx, "n.\r\n");

    // Zoom from a word+space state through the paragraph node.
    REQUIRE(dasher_seed_buffer(ctx, "nq ", 3) == 0);
    dasher_node_info nodes[256];
    dasher_mouse_down(ctx);
    bool entered = false;
    for (int i = 0; i < 900 && !entered; i++) {
        for (int k = 0; k < 256; k++)
            nodes[k].struct_size = sizeof(dasher_node_info);
        char** strs = nullptr;
        int strCount = 0;
        // dasher_get_visible_nodes returns the TOTAL count, not the number
        // written into the buffer — every read is clamped to the array
        // bound (greptile P2: reading past nodes[256] on busy frames).
        const int n = dasher_get_visible_nodes(ctx, nodes, 256, &strs, &strCount);
        int best = -1;
        long long bestArea = -1;
        for (int k = 0; k < n && k < 256; k++) {
            if (nodes[k].symbol != paraSym) continue;
            const long long area =
                (long long)(nodes[k].screen_x2 - nodes[k].screen_x1) * (nodes[k].screen_y2 - nodes[k].screen_y1);
            if (area > bestArea) {
                bestArea = area;
                best = k;
            }
        }
        if (best >= 0) {
            const float cx = (nodes[best].screen_x1 + nodes[best].screen_x2) / 2.0f;
            const float cy = (nodes[best].screen_y1 + nodes[best].screen_y2) / 2.0f;
            dasher_mouse_move(ctx, cx, cy);
        } else {
            dasher_mouse_move(ctx, 700.0f, 300.0f); // neutral hold
        }
        run_frames(ctx, 1, 1000 + (int64_t)i * 16);
        const char* out = dasher_get_output_text(ctx);
        entered = out && strchr(out, '\n');
    }
    dasher_mouse_up(ctx);
    REQUIRE(entered);

    // The paragraph node just entered is the root: its children's
    // probabilities must be the canonical paragraph-start distribution.
    int lb[64], hb[64];
    int n = dasher_get_probabilities(ctx, lb, hb, 64);
    REQUIRE(n > 0);
    std::vector<int> live;
    for (int i = 0; i < n; i++)
        live.push_back(hb[i] - lb[i]);
    REQUIRE(live == canonical);
}
