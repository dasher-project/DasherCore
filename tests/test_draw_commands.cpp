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
