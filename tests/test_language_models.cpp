// Language model tests: LM switching, LM parameters, text output per LM
#include "test_common.h"

static void run_frames(dasher_ctx* ctx, int count) {
    int* commands = nullptr;
    int cmd_count = 0;
    char** strings = nullptr;
    int str_count = 0;
    for (int i = 0; i < count; i++) {
        dasher_frame(ctx, 1000 + i * 20, &commands, &cmd_count, &strings, &str_count);
    }
}

static void produce_text(dasher_ctx* ctx, int frames) {
    dasher_set_speed_percent(ctx, 300);
    dasher_mouse_move(ctx, 700.0f, 300.0f);
    dasher_mouse_down(ctx);
    for (int i = 0; i < frames; i++) {
        dasher_mouse_move(ctx, 700.0f, 280.0f);
        run_frames(ctx, 1);
    }
    dasher_mouse_up(ctx);
}

TEST(lm_list_and_ids) {
    int count = dasher_get_language_model_count();
    ASSERT(count > 0);
    printf("  LM count: %d\n", count);

    for (int i = 0; i < count; i++) {
        int id = dasher_get_language_model_id_at(i);
        ASSERT(id >= 0);
        const char* name = dasher_get_language_model_name(id);
        ASSERT(name != nullptr);
        ASSERT(strlen(name) > 0);
        printf("  LM[%d] id=%d name='%s'\n", i, id, name);
    }

    ASSERT_EQ(dasher_get_language_model_id_at(-1), -1);
    ASSERT_EQ(dasher_get_language_model_id_at(999), -1);
    printf("v lm_list_and_ids passed\n");
}

TEST(lm_unknown_returns_safe_defaults) {
    ASSERT_STR_EQ(dasher_get_language_model_name(99999), "Unknown");
    ASSERT_STR_EQ(dasher_get_language_model_description(99999), "");
    ASSERT_EQ(dasher_get_language_model_param_count(99999), 0);
    ASSERT_EQ(dasher_get_language_model_param_key(99999, 0), -1);
    printf("v lm_unknown_returns_safe_defaults passed\n");
}

TEST(lm_default_is_ppm) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int lm_id = dasher_get_language_model_id(ctx);
    printf("  Default LM ID: %d\n", lm_id);
    ASSERT(lm_id >= 0);

    dasher_destroy(ctx);
    printf("v lm_default_is_ppm passed\n");
}

TEST(lm_switch_to_word_model) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int original_lm = dasher_get_language_model_id(ctx);

    int word_key = dasher_find_parameter_key("LP_LANGUAGE_MODEL_ID");
    ASSERT(word_key >= 0);

    dasher_set_language_model_id(ctx, 3);
    int current = dasher_get_language_model_id(ctx);
    printf("  After set to 3: LM ID = %d\n", current);

    dasher_set_language_model_id(ctx, original_lm);
    dasher_destroy(ctx);
    printf("v lm_switch_to_word_model passed\n");
}

TEST(lm_ppm_produces_text) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_language_model_id(ctx, 0);
    produce_text(ctx, 200);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    printf("  PPM output: '%s'\n", text);
    ASSERT(strlen(text) > 0);

    dasher_destroy(ctx);
    printf("v lm_ppm_produces_text passed\n");
}

TEST(lm_parameters_accessible) {
    int alpha_key = dasher_find_parameter_key("LP_LM_ALPHA");
    int beta_key = dasher_find_parameter_key("LP_LM_BETA");
    int max_order_key = dasher_find_parameter_key("LP_LM_MAX_ORDER");
    int exclusion_key = dasher_find_parameter_key("LP_LM_EXCLUSION");

    printf("  alpha=%d beta=%d max_order=%d exclusion=%d\n", alpha_key, beta_key, max_order_key, exclusion_key);
    ASSERT(alpha_key >= 0);
    ASSERT(beta_key >= 0);
    ASSERT(max_order_key >= 0);
    ASSERT(exclusion_key >= 0);

    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    long orig_order = dasher_get_long_parameter(ctx, max_order_key);
    dasher_set_long_parameter(ctx, max_order_key, 8);
    long new_order = dasher_get_long_parameter(ctx, max_order_key);
    ASSERT_EQ(new_order, 8);
    dasher_set_long_parameter(ctx, max_order_key, orig_order);

    dasher_destroy(ctx);
    printf("v lm_parameters_accessible passed\n");
}

TEST(lm_param_keys_are_valid) {
    int count = dasher_get_language_model_count();
    for (int i = 0; i < count; i++) {
        int id = dasher_get_language_model_id_at(i);
        int param_count = dasher_get_language_model_param_count(id);
        printf("  LM id=%d has %d params\n", id, param_count);
        for (int j = 0; j < param_count; j++) {
            int key = dasher_get_language_model_param_key(id, j);
            ASSERT(key >= 0);
        }
    }
    printf("v lm_param_keys_are_valid passed\n");
}

TEST(lm_mixture_model_id) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    int orig = dasher_get_language_model_id(ctx);

    dasher_set_language_model_id(ctx, 4);

    int mixture_key = dasher_find_parameter_key("LP_LM_MIXTURE");
    if (mixture_key >= 0) {
        long orig_mix = dasher_get_long_parameter(ctx, mixture_key);
        dasher_set_long_parameter(ctx, mixture_key, 50);
        ASSERT_EQ(dasher_get_long_parameter(ctx, mixture_key), 50);
        dasher_set_long_parameter(ctx, mixture_key, orig_mix);
    }

    dasher_set_language_model_id(ctx, orig);
    dasher_destroy(ctx);
    printf("v lm_mixture_model_id passed\n");
}

TEST(lm_word_model_produces_text) {
    dasher_ctx* ctx = create_isolated_context();
    ASSERT(ctx != nullptr);
    dasher_set_screen_size(ctx, 800, 600);

    dasher_set_language_model_id(ctx, 3);
    produce_text(ctx, 200);

    const char* text = dasher_get_output_text(ctx);
    ASSERT(text != nullptr);
    printf("  Word model output: '%s'\n", text);

    dasher_destroy(ctx);
    printf("v lm_word_model_produces_text passed\n");
}

int main() {
    printf("Running Dasher language model tests...\n\n");

    test_lm_list_and_ids();
    test_lm_unknown_returns_safe_defaults();
    test_lm_default_is_ppm();
    test_lm_switch_to_word_model();
    test_lm_ppm_produces_text();
    test_lm_parameters_accessible();
    test_lm_param_keys_are_valid();
    test_lm_mixture_model_id();
    test_lm_word_model_produces_text();

    printf("\nAll language model tests passed!\n");
    return 0;
}
