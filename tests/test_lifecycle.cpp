// Lifecycle and robustness tests: multi-context, long sessions, resize, reinit
#include "test_common.h"
#include <cmath>
#include <initializer_list>

static void run_frames(dasher_ctx* ctx, int count, int64_t start_ms) {
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    for (int i = 0; i < count; i++) {
        dasher_frame(ctx, start_ms + i * 16, &commands, &cmd_count, &strings, &str_count);
    }
}

TEST(lifecycle_create_destroy_recreate) {
    for (int round = 0; round < 3; round++) {
        dasher_ctx* ctx = create_isolated_context();
        ASSERT(ctx != nullptr);
        dasher_set_screen_size(ctx, 800, 600);

        int* cmds; int cmd_count; char** strs; int str_count;
        dasher_frame(ctx, 1000, &cmds, &cmd_count, &strs, &str_count);
        ASSERT(cmd_count > 0);

        dasher_destroy(ctx);
        printf("  Round %d: ok\n", round);
    }
    printf("v lifecycle_create_destroy_recreate passed\n");
}

TEST(lifecycle_multiple_contexts) {
    dasher_ctx* ctx1 = create_isolated_context();
    dasher_ctx* ctx2 = create_isolated_context();
    ASSERT(ctx1 != nullptr);
    ASSERT(ctx2 != nullptr);
    ASSERT(ctx1 != ctx2);

    dasher_set_screen_size(ctx1, 800, 600);
    dasher_set_screen_size(ctx2, 400, 300);

    dasher_set_speed_percent(ctx1, 100);
    dasher_set_speed_percent(ctx2, 200);

    ASSERT_NEQ(dasher_get_speed_percent(ctx1), dasher_get_speed_percent(ctx2));
    printf("  ctx1 speed: %d, ctx2 speed: %d\n",
           dasher_get_speed_percent(ctx1), dasher_get_speed_percent(ctx2));

    dasher_destroy(ctx1);
    dasher_destroy(ctx2);
    printf("v lifecycle_multiple_contexts passed\n");
}

TEST(lifecycle_long_session) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 200);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);

    int last_len = 0;
    for (int i = 0; i < 2000; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f + (float)(i % 20));
        run_frames(ctx, 1, i * 16);

        if (i % 500 == 499) {
            const char* text = dasher_get_output_text(ctx);
            int len = (int)strlen(text);
            printf("  Frame %d: output len=%d\n", i + 1, len);
            ASSERT(len >= last_len || len == 0);
            last_len = len;
        }
    }

    dasher_mouse_up(ctx);
    const char* final_text = dasher_get_output_text(ctx);
    printf("  Final output length: %zu\n", strlen(final_text));

    dasher_destroy(ctx);
    printf("v lifecycle_long_session passed\n");
}

TEST(lifecycle_screen_resize) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);

    dasher_set_screen_size(ctx, 800, 600);
    run_frames(ctx, 5, 1000);

    dasher_set_screen_size(ctx, 400, 300);
    int* cmds; int cmd_count; char** strs; int str_count;
    dasher_frame(ctx, 2000, &cmds, &cmd_count, &strs, &str_count);
    ASSERT(cmd_count > 0);
    printf("  After resize to 400x300: %d commands\n", cmd_count);

    dasher_set_screen_size(ctx, 1024, 768);
    dasher_frame(ctx, 3000, &cmds, &cmd_count, &strs, &str_count);
    ASSERT(cmd_count > 0);
    printf("  After resize to 1024x768: %d commands\n", cmd_count);

    dasher_destroy(ctx);
    printf("v lifecycle_screen_resize passed\n");
}

TEST(lifecycle_rapid_mouse_movement) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        float x = 400.0f + 300.0f * (float)sin(i * 0.1);
        float y = 300.0f + 200.0f * (float)cos(i * 0.15);
        dasher_mouse_move(ctx, x, y);
        run_frames(ctx, 1, i * 16);
    }
    dasher_mouse_up(ctx);

    printf("  Survived 500 rapid mouse moves\n");
    dasher_destroy(ctx);
    printf("v lifecycle_rapid_mouse_movement passed\n");
}

TEST(lifecycle_game_mode_repeated) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    for (int round = 0; round < 3; round++) {
        ASSERT_EQ(dasher_game_mode_active(ctx), 0);
        int result = dasher_enter_game_mode(ctx);
        ASSERT_EQ(result, 0);
        ASSERT_EQ(dasher_game_mode_active(ctx), 1);

        const char* target = dasher_game_get_target_text(ctx);
        ASSERT(target != nullptr);
        printf("  Round %d target: '%s'\n", round, target);

        dasher_leave_game_mode(ctx);
        ASSERT_EQ(dasher_game_mode_active(ctx), 0);
    }

    dasher_destroy(ctx);
    printf("v lifecycle_game_mode_repeated passed\n");
}

TEST(lifecycle_speed_changes_mid_session) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    for (int speed : {50, 100, 200, 300, 400, 100}) {
        dasher_set_speed_percent(ctx, speed);
        int actual = dasher_get_speed_percent(ctx);
        ASSERT(actual >= 20 && actual <= 400);

        dasher_mouse_move(ctx, 700.0f, 300.0f);
        dasher_mouse_down(ctx);
        run_frames(ctx, 20, speed * 1000);
        dasher_mouse_up(ctx);
    }

    dasher_destroy(ctx);
    printf("v lifecycle_speed_changes_mid_session passed\n");
}

TEST(lifecycle_output_callback_userdata) {
    struct CallbackData {
        int event_count;
        char last_text[64];
    };
    CallbackData data = {0, ""};

    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_output_callback(ctx, [](int event_type, const char* text, void* user_data) {
        CallbackData* d = static_cast<CallbackData*>(user_data);
        if (event_type == 0 && text) {
            d->event_count++;
            strncpy(d->last_text, text, 63);
            d->last_text[63] = '\0';
        }
    }, &data);

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, i * 16);
    }
    dasher_mouse_up(ctx);

    printf("  Callback fired %d times, last text: '%s'\n", data.event_count, data.last_text);
    ASSERT(data.event_count > 0);

    dasher_destroy(ctx);
    printf("v lifecycle_output_callback_userdata passed\n");
}

TEST(lifecycle_message_callback_userdata) {
    struct MsgData { int count; char last_msg[256]; };
    MsgData msg = {0, ""};

    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_message_callback(ctx, [](int type, const char* text, void* user_data) {
        MsgData* d = static_cast<MsgData*>(user_data);
        d->count++;
        if (text) {
            strncpy(d->last_msg, text, 255);
            d->last_msg[255] = '\0';
        }
    }, &msg);

    dasher_set_alphabet_id(ctx, "Nonexistent Alphabet");
    run_frames(ctx, 5, 1000);

    printf("  Messages received: %d\n", msg.count);

    dasher_destroy(ctx);
    printf("v lifecycle_message_callback_userdata passed\n");
}

TEST(lifecycle_reset_clears_state) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 300; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, i * 16);
    }
    dasher_mouse_up(ctx);

    const char* text1 = dasher_get_output_text(ctx);
    printf("  Before reset: '%s' (len=%zu)\n", text1, strlen(text1));
    ASSERT(strlen(text1) > 0);

    dasher_reset(ctx);
    const char* text2 = dasher_get_output_text(ctx);
    printf("  After reset: '%s' (len=%zu)\n", text2, strlen(text2));
    ASSERT_EQ(strlen(text2), 0);

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 500; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, 100000 + i * 16);
    }
    dasher_mouse_up(ctx);

    const char* text3 = dasher_get_output_text(ctx);
    printf("  After re-type: '%s' (len=%zu)\n", text3, strlen(text3));

    dasher_destroy(ctx);
    printf("v lifecycle_reset_clears_state passed\n");
}

TEST(lifecycle_speak_callback_registration) {
    struct SpeakData { int count; char last_text[256]; };
    SpeakData data = {0, ""};

    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_speak_callback(ctx, [](const char* text, int interrupt, void* user_data) {
        SpeakData* d = static_cast<SpeakData*>(user_data);
        d->count++;
        if (text) {
            strncpy(d->last_text, text, 255);
            d->last_text[255] = '\0';
        }
    }, &data);

    int speak_words_key = dasher_find_parameter_key("BP_SPEAK_WORDS");
    if (speak_words_key >= 0) {
        dasher_set_bool_parameter(ctx, speak_words_key, 1);
    }

    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < 200; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1, i * 16);
    }
    dasher_mouse_up(ctx);

    printf("  Speak callback fired: %d times\n", data.count);

    dasher_destroy(ctx);
    printf("v lifecycle_speak_callback_registration passed\n");
}

int main() {
    printf("Running Dasher lifecycle and robustness tests...\n\n");

    test_lifecycle_create_destroy_recreate();
    test_lifecycle_multiple_contexts();
    test_lifecycle_long_session();
    test_lifecycle_screen_resize();
    test_lifecycle_rapid_mouse_movement();
    test_lifecycle_game_mode_repeated();
    test_lifecycle_speed_changes_mid_session();
    test_lifecycle_output_callback_userdata();
    test_lifecycle_message_callback_userdata();
    test_lifecycle_reset_clears_state();
    test_lifecycle_speak_callback_registration();

    printf("\nAll lifecycle tests passed!\n");
    return 0;
}
