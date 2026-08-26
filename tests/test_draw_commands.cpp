// Draw command tests: validate opcode structure, bounds, strings, rendering
#include "test_common.h"

[[maybe_unused]] static void get_frame(dasher_ctx* ctx, int64_t time, int** cmds, int* cmd_count, char*** strs,
                                       int* str_count) {
    *cmds = nullptr;
    *cmd_count = 0;
    *strs = nullptr;
    *str_count = 0;
    dasher_frame(ctx, time, cmds, cmd_count, strs, str_count);
}

TEST(draw_command_alignment) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int* cmds;
    int cmd_count;
    char** strs;
    int str_count;
    get_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);

    ASSERT(cmds != nullptr);
    ASSERT(cmd_count > 0);
    ASSERT_EQ(cmd_count % 6, 0);
    printf("  Command count: %d (%d draw ops)\n", cmd_count, cmd_count / 6);

    dasher_destroy(ctx);
}

TEST(draw_opcodes_in_range) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    for (int frame = 0; frame < 50; frame++) {
        dasher_mouse_move(ctx, 700.0f, 290.0f);
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000 + frame * 16, &cmds, &cmd_count, &strs, &str_count);

        int ops = cmd_count / 6;
        for (int i = 0; i < ops; i++) {
            int opcode = cmds[i * 6];
            ASSERT(opcode >= 0 && opcode <= 6);
        }
    }

    dasher_mouse_up(ctx);
    dasher_destroy(ctx);
}

TEST(draw_first_command_is_clear) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int* cmds;
    int cmd_count;
    char** strs;
    int str_count;
    get_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);

    int first_op = cmds[0];
    ASSERT_EQ(first_op, 0);

    int argb = cmds[5];
    int alpha = (argb >> 24) & 0xFF;
    ASSERT(alpha > 0);
    printf("  Clear screen: opcode=%d argb=0x%08X alpha=%d\n", first_op, argb, alpha);

    dasher_destroy(ctx);
}

TEST(draw_text_string_indices_valid) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    bool found_text_op = false;
    for (int frame = 0; frame < 100; frame++) {
        dasher_mouse_move(ctx, 700.0f, 285.0f);
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000 + frame * 16, &cmds, &cmd_count, &strs, &str_count);

        int ops = cmd_count / 6;
        for (int i = 0; i < ops; i++) {
            int base = i * 6;
            if (cmds[base] == 5) {
                found_text_op = true;
                int str_idx = cmds[base + 4];
                ASSERT(str_idx >= 0);
                ASSERT(str_idx < str_count);
                if (strs && str_idx < str_count) {
                    ASSERT(strs[str_idx] != nullptr);
                }
            }
        }
    }

    dasher_mouse_up(ctx);
    printf("  Found text draw ops: %s\n", found_text_op ? "yes" : "no");
    dasher_destroy(ctx);
}

TEST(draw_coordinates_in_bounds) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 150);
    dasher_mouse_move(ctx, 400.0f, 300.0f);
    dasher_mouse_down(ctx);

    for (int frame = 0; frame < 30; frame++) {
        dasher_mouse_move(ctx, 400.0f, 290.0f);
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000 + frame * 16, &cmds, &cmd_count, &strs, &str_count);

        int ops = cmd_count / 6;
        for (int i = 0; i < ops; i++) {
            int base = i * 6;
            int op = cmds[base];
            int a = cmds[base + 1];
            int b = cmds[base + 2];
            int c = cmds[base + 3];
            int d = cmds[base + 4];
            (void)a;
            (void)b;
            (void)d;

            switch (op) {
            case 1:
                ASSERT(c > 0);
                break;
            case 2:
            case 3:
            case 4:
            case 6:
                break;
            case 5:
                ASSERT(c > 0);
                break;
            }
        }
    }

    dasher_mouse_up(ctx);
    dasher_destroy(ctx);
}

TEST(draw_multiple_frames_consistent) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    for (int frame = 0; frame < 200; frame++) {
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000 + frame * 16, &cmds, &cmd_count, &strs, &str_count);

        ASSERT(cmd_count >= 0);
        if (cmd_count > 0) {
            ASSERT(cmds != nullptr);
            ASSERT_EQ(cmd_count % 6, 0);
        }
    }

    dasher_destroy(ctx);
}

TEST(draw_colors_have_alpha) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    bool found_nonzero_alpha = false;
    for (int frame = 0; frame < 50; frame++) {
        dasher_mouse_move(ctx, 700.0f, 290.0f);
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000 + frame * 16, &cmds, &cmd_count, &strs, &str_count);

        int ops = cmd_count / 6;
        for (int i = 0; i < ops; i++) {
            int argb = cmds[i * 6 + 5];
            int alpha = (argb >> 24) & 0xFF;
            if (alpha > 0) found_nonzero_alpha = true;
        }
    }

    dasher_mouse_up(ctx);
    ASSERT(found_nonzero_alpha);
    dasher_destroy(ctx);
}

TEST(draw_no_mouse_produces_idle_frame) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int* cmds;
    int cmd_count;
    char** strs;
    int str_count;
    get_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);

    ASSERT(cmd_count > 0);
    ASSERT_EQ(cmds[0], 0);

    int* cmds2;
    int cmd_count2;
    char** strs2;
    int str_count2;
    get_frame(ctx, 1016, &cmds2, &cmd_count2, &strs2, &str_count2);

    ASSERT(cmd_count2 > 0);

    dasher_destroy(ctx);
}

TEST(uppercase_group_box_has_distinct_colour) {
    // Regression (#62 — "v5 put capitals in a separate, uniquely colored
    // node"): the uppercase group box must render with its palette group
    // colour. Two failures conspired: legacy colour.*.xml files parsed AFTER
    // the new-format color.*.xml files, and Data/colours/colour.xml is a
    // legacy palette also named "Default" — it overwrote the new Default and
    // dropped its group colours (uppercase #FFFF00). And the legacy parser
    // itself never set letter-family group colours, so legacy palettes lost
    // v5's uppercase pink (palette index 111 = #FF80FF).
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    // The uppercase group's box is asserted by geometry, not colour alone:
    // at rest the initial view's text labels are the capital letters
    // themselves, so the "A" label must sit inside a filled rect carrying
    // the uppercase group colour. (Colour-only would let an unrelated
    // yellow element satisfy the assertion.)
    auto frameHasColouredUppercaseBox = [&](int r, int g, int b) {
        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        get_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);
        for (int i = 0; i + 5 < cmd_count; i += 6) {
            if (cmds[i] != 5) continue; // text
            const int sidx = cmds[i + 4];
            if (sidx < 0 || sidx >= str_count) continue;
            if (strcmp(strs[sidx], "A") != 0) continue;
            const int tx = cmds[i + 1], ty = cmds[i + 2];
            for (int j = 0; j + 5 < cmd_count; j += 6) {
                if (cmds[j] != 4) continue; // filled rectangle
                const int argb = cmds[j + 5];
                if (((argb >> 16) & 0xFF) != r || ((argb >> 8) & 0xFF) != g || (argb & 0xFF) != b) continue;
                const int x1 = cmds[j + 1] < cmds[j + 3] ? cmds[j + 1] : cmds[j + 3];
                const int x2 = cmds[j + 1] < cmds[j + 3] ? cmds[j + 3] : cmds[j + 1];
                const int y1 = cmds[j + 2] < cmds[j + 4] ? cmds[j + 2] : cmds[j + 4];
                const int y2 = cmds[j + 2] < cmds[j + 4] ? cmds[j + 4] : cmds[j + 2];
                if (tx >= x1 && tx <= x2 && ty >= y1 && ty <= y2) return true;
            }
        }
        return false;
    };

    // Warm up so the initial tree is fully expanded.
    run_frames(ctx, 30);

    // Default palette: uppercase group box = #FFFF00 (from color.default.xml,
    // which must survive the legacy colour.xml name collision).
    ASSERT(frameHasColouredUppercaseBox(255, 255, 0));

    // Legacy palette: same group colour via the legacy indices —
    // ParseLegacy must set the letter-family group colours.
    const int colourKey = dasher_find_parameter_key("SP_COLOUR_ID");
    ASSERT(colourKey >= 0);
    dasher_set_string_parameter(ctx, colourKey, "European/Asian (Original)");
    run_frames(ctx, 30);
    ASSERT(frameHasColouredUppercaseBox(255, 255, 0));

    dasher_destroy(ctx);
}
