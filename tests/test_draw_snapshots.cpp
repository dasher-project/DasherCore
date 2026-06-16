// Draw command snapshot tests: deterministic command sequences for rewrite validation
#include "test_common.h"
#include <string.h>

static unsigned long hash_commands(const int* cmds, int count) {
    unsigned long h = 5381;
    for (int i = 0; i < count; i++)
        h = ((h << 5) + h) + (unsigned int)cmds[i];
    return h;
}

TEST(snapshot_frame0_deterministic) {
    unsigned long hashes[5];
    for (int run = 0; run < 5; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);

        int* cmds;
        int cmd_count;
        char** strs;
        int str_count;
        dasher_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);
        hashes[run] = hash_commands(cmds, cmd_count);
        printf("  Run %d: %d cmds, hash=%lu\n", run, cmd_count, hashes[run]);
        dasher_destroy(ctx);
    }

    for (int i = 1; i < 5; i++)
        ASSERT_EQ(hashes[i], hashes[0]);
    printf("  Hash: %lu (deterministic across 5 runs)\n", hashes[0]);

    printf("v snapshot_frame0_deterministic passed\n");
}

TEST(snapshot_frame10_deterministic) {
    unsigned long hashes[3];
    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_speed_percent(ctx, 200);

        int* cmds = nullptr;
        int cmd_count = 0;
        char** strs = nullptr;
        int str_count = 0;
        for (int f = 0; f < 10; f++)
            dasher_frame(ctx, 1000 + f * 16, &cmds, &cmd_count, &strs, &str_count);

        hashes[run] = hash_commands(cmds, cmd_count);
        dasher_destroy(ctx);
    }

    for (int i = 1; i < 3; i++)
        ASSERT_EQ(hashes[i], hashes[0]);
    printf("  Frame 10 hash: %lu (deterministic)\n", hashes[0]);

    printf("v snapshot_frame10_deterministic passed\n");
}

TEST(snapshot_with_mouse_input_deterministic) {
    unsigned long hashes[3];
    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_speed_percent(ctx, 200);

        dasher_mouse_move(ctx, 600.0f, 300.0f);
        dasher_mouse_down(ctx);

        int* cmds = nullptr;
        int cmd_count = 0;
        char** strs = nullptr;
        int str_count = 0;
        for (int f = 0; f < 20; f++) {
            dasher_mouse_move(ctx, 600.0f, 290.0f);
            dasher_frame(ctx, 1000 + f * 16, &cmds, &cmd_count, &strs, &str_count);
        }
        hashes[run] = hash_commands(cmds, cmd_count);
        dasher_mouse_up(ctx);
        dasher_destroy(ctx);
    }

    ASSERT_EQ(hashes[1], hashes[0]);
    ASSERT_EQ(hashes[2], hashes[0]);
    printf("  Mouse-input frame 20 hash: %lu\n", hashes[0]);

    printf("v snapshot_with_mouse_input_deterministic passed\n");
}

TEST(snapshot_command_structure_consistent) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx);
    dasher_set_screen_size(ctx, 800, 600);
    dasher_set_speed_percent(ctx, 150);

    int* cmds;
    int cmd_count;
    char** strs;
    int str_count;
    dasher_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);

    ASSERT(cmd_count > 0);
    ASSERT_EQ(cmd_count % 6, 0);
    ASSERT_EQ(cmds[0], 0);

    int ops = cmd_count / 6;
    int clear_count = 0, circle_count = 0, line_count = 0;
    int rect_outline = 0, rect_filled = 0, text_count = 0;

    for (int i = 0; i < ops; i++) {
        int op = cmds[i * 6];
        switch (op) {
        case 0:
            clear_count++;
            break;
        case 1:
            circle_count++;
            break;
        case 2:
            line_count++;
            break;
        case 3:
            rect_outline++;
            break;
        case 4:
            rect_filled++;
            break;
        case 5:
            text_count++;
            break;
        }
    }
    printf("  Ops: clear=%d circle=%d line=%d rect_o=%d rect_f=%d text=%d\n", clear_count, circle_count, line_count,
           rect_outline, rect_filled, text_count);
    ASSERT(clear_count >= 1);

    dasher_destroy(ctx);
    printf("v snapshot_command_structure_consistent passed\n");
}

TEST(snapshot_output_text_deterministic) {
    char outputs[3][1024];

    for (int run = 0; run < 3; run++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx);
        dasher_set_screen_size(ctx, 800, 600);
        dasher_set_speed_percent(ctx, 300);

        dasher_mouse_move(ctx, 700.0f, 300.0f);
        dasher_mouse_down(ctx);
        int* c = nullptr;
        int cc = 0;
        char** s = nullptr;
        int sc = 0;
        for (int i = 0; i < 500; i++) {
            dasher_mouse_move(ctx, 700.0f, 280.0f);
            dasher_frame(ctx, 1000 + i * 16, &c, &cc, &s, &sc);
        }
        dasher_mouse_up(ctx);

        strncpy(outputs[run], dasher_get_output_text(ctx), 1023);
        outputs[run][1023] = '\0';
        printf("  Run %d output: '%s'\n", run, outputs[run]);
        dasher_destroy(ctx);
    }

    ASSERT_EQ(strcmp(outputs[0], outputs[1]), 0);
    ASSERT_EQ(strcmp(outputs[0], outputs[2]), 0);
    printf("  Output is deterministic across 3 runs\n");

    printf("v snapshot_output_text_deterministic passed\n");
}

int main() {
    printf("Running draw command snapshot tests...\n\n");

    test_snapshot_frame0_deterministic();
    test_snapshot_frame10_deterministic();
    test_snapshot_with_mouse_input_deterministic();
    test_snapshot_command_structure_consistent();
    test_snapshot_output_text_deterministic();

    printf("\nAll draw snapshot tests passed!\n");
    return 0;
}
