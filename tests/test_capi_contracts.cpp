// test_capi_contracts.cpp
//
// CHARACTERIZATION TESTS for the C API's error/return conventions and
// process-global state (todo.md Phase 0.5).
//
// The API currently mixes conventions: some functions signal errors with
// 0/-1 ints, some return "" and some NULL for string failures, and -1 means
// "invalid state" for some getters. These tests snapshot the CURRENT
// conventions per function so that:
//   - accidental drift is caught immediately, and
//   - a deliberate unification (todo.md Phase 4) is a conscious, versioned
//     change rather than a silent surprise for frontends.
//
// Also covers: NULL-ctx tolerance (the whole API accepts a null ctx without
// crashing) and the process-global locale/override state (two contexts share
// one locale — a known wart, documented deliberately here so a future
// per-context fix is a deliberate change).

#include "test_common.h"

#include <string>

// ---------------------------------------------------------------------------
// Version pin — the compile-time constant and the runtime function must
// agree, or frontends gate workarounds on the wrong number.
// ---------------------------------------------------------------------------

TEST_CASE("contracts/capi version matches header constant") {
    CHECK(dasher_capi_version() == DASHER_CAPI_VERSION);
}

// ---------------------------------------------------------------------------
// NULL-ctx tolerance. Every entry point must accept a null ctx and no-op
// (or return its documented default) — frontends routinely call getters
// during teardown races. dasher_frame must additionally zero its out params.
// ---------------------------------------------------------------------------

TEST_CASE("contracts/null ctx tolerated across the API surface") {
    // Setters / void functions: must not crash.
    dasher_destroy(nullptr);
    dasher_set_low_memory_mode(nullptr, 1);
    dasher_set_screen_size(nullptr, 100, 100);
    dasher_mouse_move(nullptr, 1.0f, 2.0f);
    dasher_mouse_down(nullptr);
    dasher_mouse_up(nullptr);
    dasher_key_event(nullptr, 100, 1);
    dasher_reset(nullptr);
    dasher_reset_output_text(nullptr);
    dasher_reset_cps(nullptr);
    dasher_save_settings(nullptr);
    dasher_reload_settings(nullptr);
    dasher_reset_settings(nullptr);
    dasher_set_palette(nullptr, "Default");
    dasher_set_user_palette(nullptr, "Default");
    dasher_set_light_palette(nullptr, "Default");
    dasher_set_dark_palette(nullptr, "Default");
    dasher_set_appearance_mode(nullptr, 1);
    dasher_set_system_appearance(nullptr, 2);
    dasher_set_alphabet_id(nullptr, "English");
    dasher_set_language_model_id(nullptr, 1);
    dasher_set_speed_percent(nullptr, 200);
    dasher_set_bool_parameter(nullptr, 0, 1);
    dasher_set_long_parameter(nullptr, 0, 1);
    dasher_set_string_parameter(nullptr, 0, "x");
    dasher_set_locale(nullptr, "en");
    dasher_set_string_override(nullptr, "k", "v");
    dasher_register_action(nullptr, "a", nullptr, nullptr);
    dasher_set_offset(nullptr, 0);
    dasher_seed_buffer(nullptr, "text", 0);
    dasher_text_metrics_changed(nullptr);
    dasher_set_output_callback(nullptr, nullptr, nullptr);
    dasher_set_text_size_callback(nullptr, nullptr, nullptr);
    dasher_set_message_callback(nullptr, nullptr, nullptr);
    dasher_set_log_callback(nullptr, nullptr, nullptr, 0);
    dasher_set_speak_callback(nullptr, nullptr, nullptr);
    dasher_set_clipboard_callback(nullptr, nullptr, nullptr);
    dasher_set_parameter_callback(nullptr, nullptr, nullptr);
    dasher_enter_game_mode(nullptr);
    dasher_leave_game_mode(nullptr);
    dasher_game_set_canvas_text(nullptr, 1);
    dasher_set_visible_nodes_enabled(nullptr, 1);
    dasher_import_training_text(nullptr, "text");

    // Getters: documented defaults, never a crash.
    CHECK(std::string(dasher_get_output_text(nullptr)) == "");
    CHECK(std::string(dasher_get_locale(nullptr)) == "en");
    CHECK(dasher_get_speed_percent(nullptr) == 100);
    CHECK(dasher_get_appearance_mode(nullptr) == 0);
    CHECK(dasher_get_system_appearance(nullptr) == 1);
    CHECK(dasher_get_cps(nullptr) == 0.0);
    CHECK(dasher_get_wpm(nullptr) == 0.0);
    CHECK(dasher_has_engine_error(nullptr) == 0);
    CHECK(dasher_get_bool_parameter(nullptr, 0) == 0);
    CHECK(dasher_get_long_parameter(nullptr, 0) == 0);
    CHECK(std::string(dasher_get_string_parameter(nullptr, 0)) == "");
    CHECK(dasher_get_parameter_string_values(nullptr, 0, nullptr, 0) == 0);
    CHECK(dasher_get_palette_count(nullptr) == 0);
    CHECK(std::string(dasher_get_palette_name(nullptr, 0)) == "");
    CHECK(std::string(dasher_get_current_palette(nullptr)) == "");
    CHECK(dasher_get_palette_appearance(nullptr, 0) == -1);
    CHECK(dasher_find_companion_palette(nullptr, "Default") == nullptr);
    CHECK(std::string(dasher_get_light_palette(nullptr)) == "");
    CHECK(std::string(dasher_get_dark_palette(nullptr)) == "");
    CHECK(dasher_get_alphabet_count(nullptr) == 0);
    CHECK(std::string(dasher_get_alphabet_name(nullptr, 0)) == "");
    CHECK(std::string(dasher_get_alphabet_id(nullptr)) == "");
    CHECK(dasher_get_language_model_id(nullptr) == 0);
    CHECK(dasher_game_mode_active(nullptr) == 0);
    CHECK(std::string(dasher_game_get_target_text(nullptr)) == "");
    CHECK(dasher_game_get_correct_count(nullptr) == -1);
    CHECK(dasher_game_get_target_length(nullptr) == -1);
    CHECK(std::string(dasher_game_get_wrong_text(nullptr)) == "");
    CHECK(dasher_get_offset(nullptr) == -1);
    CHECK(dasher_get_alphabet_symbol_count(nullptr) == -1);
    char buf[64];
    CHECK(dasher_get_alphabet_symbol_text(nullptr, 0, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_alphabet_symbol_display(nullptr, 0, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_alphabet_symbol_image(nullptr, 0, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_probabilities(nullptr, nullptr, nullptr, 0) == -1);
    CHECK(dasher_screen_to_dasher(nullptr, 0, 0, nullptr, nullptr) == -1);
    CHECK(dasher_dasher_to_screen(nullptr, 0, 0, nullptr, nullptr) == -1);
    CHECK(dasher_get_root_child_count(nullptr) == -1);
    CHECK(dasher_get_root_child_bounds(nullptr, 0, nullptr, nullptr) == -1);
    CHECK(dasher_get_viewport(nullptr, nullptr) == -1);
    CHECK(dasher_get_visible_nodes(nullptr, nullptr, 0, nullptr, nullptr) == -1);
    CHECK(dasher_get_training_path(nullptr) != nullptr);
    CHECK(dasher_get_localized_string(nullptr, "k") == nullptr);

    // frame with null ctx: out params must be zeroed/null, not left stale.
    int* cmds = reinterpret_cast<int*>(0x1);
    int cmd_count = -1;
    char** strs = reinterpret_cast<char**>(0x1);
    int str_count = -1;
    dasher_frame(nullptr, 1000, &cmds, &cmd_count, &strs, &str_count);
    CHECK(cmds == nullptr);
    CHECK(cmd_count == 0);
    CHECK(strs == nullptr);
    CHECK(str_count == 0);
}

// ---------------------------------------------------------------------------
// Integer sentinels. -1 is the universal "error/invalid" int return; the
// per-function meaning ("out of range" vs "not realized" vs "not in game
// mode") is documented in dasher.h and pinned here.
// ---------------------------------------------------------------------------

TEST_CASE("contracts/integer error sentinels") {
    // Static lookups (no ctx).
    CHECK(dasher_find_parameter_key(nullptr) == -1);
    CHECK(dasher_find_parameter_key("LP_NOT_A_REAL_PARAMETER") == -1);
    CHECK(dasher_get_language_model_id_at(-1) == -1);
    CHECK(dasher_get_language_model_id_at(1 << 20) == -1);
    CHECK(dasher_get_language_model_param_key(-999, 0) == -1);
    CHECK(dasher_get_parameter_info(-1, nullptr) == -1);

    // Realized context: state-dependent getters.
    ScopedContext ctx(800, 600);
    dasher_parameter_info info{};
    CHECK(dasher_get_parameter_info(1 << 20, &info) == -1);
    // Pass a REAL buffer: a null out_colors returns -1 at the pointer check
    // before the index is even examined (CAPI.cpp), which would make an
    // index-related assertion vacuous.
    int colors[4];
    CHECK(dasher_get_palette_preview_colors(ctx, -1, colors) == -1);
    CHECK(dasher_get_palette_preview_colors(ctx, 1 << 20, colors) == -1);

    char buf[64];
    CHECK(dasher_get_alphabet_symbol_text(ctx, -1, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_alphabet_symbol_text(ctx, 1 << 20, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_alphabet_symbol_display(ctx, -1, buf, sizeof(buf)) == -1);
    CHECK(dasher_get_alphabet_symbol_image(ctx, -1, buf, sizeof(buf)) == -1);

    // Game getters outside game mode: -1 counts, empty strings for text
    // (the mixed convention, pinned deliberately).
    CHECK(dasher_game_mode_active(ctx) == 0);
    CHECK(dasher_game_get_correct_count(ctx) == -1);
    CHECK(dasher_game_get_target_length(ctx) == -1);
    CHECK(std::string(dasher_game_get_target_text(ctx)) == "");
    CHECK(std::string(dasher_game_get_wrong_text(ctx)) == "");

    // Offsets: invalid value rejected; valid on a realized engine.
    CHECK(dasher_set_offset(ctx, -5) == -1);
    CHECK(dasher_get_offset(ctx) >= 0);

    // Uninitialized-context sentinels (created but never given a screen).
    ScopedContext unrealized; // no dasher_set_screen_size → never realized
    CHECK(dasher_get_offset(unrealized) == -1);
    CHECK(dasher_get_probabilities(unrealized, nullptr, nullptr, 0) == -1);
    CHECK(dasher_screen_to_dasher(unrealized, 0, 0, nullptr, nullptr) == -1);
    CHECK(dasher_get_viewport(unrealized, nullptr) == -1);
    CHECK(dasher_set_visible_nodes_enabled(unrealized, 1) == -1);

    // UTF-8 conversion helpers: null text is the only -1.
    CHECK(dasher_byte_offset_from_utf16(nullptr, 0) == -1);
    CHECK(dasher_byte_offset_from_codepoints(nullptr, 0) == -1);
}

// ---------------------------------------------------------------------------
// String sentinels. "" is the common failure value; two functions return
// NULL instead (dasher_find_companion_palette, dasher_get_localized_string)
// and dasher_get_language_model_name returns "Unknown" — the outliers this
// file exists to keep visible until todo.md Phase 4 unifies them.
// ---------------------------------------------------------------------------

TEST_CASE("contracts/string error sentinels") {
    // Static LM registry accessors. With a VALID id these return pointers
    // into function-local static buffers (one each, shared across all
    // contexts — unlike tlString which is per-ctx): name and description use
    // separate statics, so a name result survives an intervening description
    // call. (The invalid-id case returns literals, which would make this pin
    // vacuous — hence the valid id here.)
    const int valid_lm = dasher_get_language_model_id_at(0);
    REQUIRE(valid_lm >= 0);
    const char* name1 = dasher_get_language_model_name(valid_lm);
    REQUIRE(std::string(name1).size() > 0);
    (void)dasher_get_language_model_description(valid_lm);
    CHECK(std::string(name1) == std::string(dasher_get_language_model_name(valid_lm))); // not clobbered
    // Invalid id sentinels: "Unknown" literal vs "" (the outlier, Phase 4).
    CHECK(std::string(dasher_get_language_model_name(-999)) == "Unknown");
    CHECK(std::string(dasher_get_language_model_description(-999)) == "");
    CHECK(std::string(dasher_get_parameter_enum_name(-999, 0)) == "");

    ScopedContext ctx(800, 600);
    CHECK(std::string(dasher_get_palette_name(ctx, -5)) == "");
    CHECK(std::string(dasher_get_palette_name(ctx, 1 << 20)) == "");
    CHECK(std::string(dasher_get_alphabet_name(ctx, 1 << 20)) == "");

    // NULL sentinels (the outliers — see file header).
    CHECK(dasher_find_companion_palette(ctx, "No Such Palette Exists") == nullptr);
    CHECK(dasher_get_localized_string(ctx, "no.such.key") == nullptr);
    CHECK(dasher_get_localized_string(ctx, nullptr) == nullptr);

    // Wrong-type parameter access: caught at the boundary, returns the
    // type's default (0 / "") rather than crashing. BP_DRAW_MOUSE_LINE is a
    // bool parameter; asking for it as long/string exercises the catch path.
    const int bool_key = dasher_find_parameter_key("BP_DRAW_MOUSE_LINE");
    REQUIRE(bool_key >= 0);
    CHECK(dasher_get_long_parameter(ctx, bool_key) == 0);
    CHECK(std::string(dasher_get_string_parameter(ctx, bool_key)) == "");
    CHECK(dasher_get_parameter_enum_count(bool_key) == 0);

    // dasher_get_training_path never returns null (may be "" pre-realize).
    CHECK(dasher_get_training_path(ctx) != nullptr);
}

// ---------------------------------------------------------------------------
// Locale / string overrides are PROCESS-GLOBAL (known wart, todo.md 5.2).
// dasher_set_locale takes a ctx but writes shared state: a locale set on
// context A is immediately visible on context B. Pinned deliberately —
// when this is fixed, this test fails and the fix ships with a
// DASHER_CAPI_VERSION bump.
// ---------------------------------------------------------------------------

TEST_CASE("contracts/locale is process-global across contexts") {
    ScopedContext a(800, 600);
    ScopedContext b(800, 600);

    // Reset on scope exit (even on assertion failure) so later test cases in
    // this binary observe the default locale.
    struct LocaleReset {
        dasher_ctx* ctx;
        ~LocaleReset() { dasher_set_locale(ctx, "en"); }
    } reset{a};

    // Default state.
    CHECK(std::string(dasher_get_locale(a)) == "en");
    CHECK(std::string(dasher_get_locale(b)) == "en");

    // strings_de.json ships in Strings/ — setting on A must succeed.
    REQUIRE(dasher_set_locale(a, "de") == 0);

    // ...and is immediately visible on B: one shared process-global.
    CHECK(std::string(dasher_get_locale(b)) == "de");

    // Overrides are equally global: set via A, readable via B.
    dasher_set_string_override(a, "test.contracts.override", "viaA");
    CHECK(std::string(dasher_get_localized_string(b, "test.contracts.override")) == "viaA");
    dasher_set_string_override(a, "test.contracts.override", nullptr); // clear

    // Unknown locale: refused, previous locale retained.
    CHECK(dasher_set_locale(a, "zz-nonexistent") == -1);
    CHECK(std::string(dasher_get_locale(b)) == "de");

    // Reset via NULL-or-"en" normalises both contexts.
    REQUIRE(dasher_set_locale(a, "en") == 0);
    CHECK(std::string(dasher_get_locale(b)) == "en");
}

// ---------------------------------------------------------------------------
// Output-callback buffer-clear event (event type 2). Header contract: any
// call that discards the edit buffer wholesale fires event 2 so subscribers
// can resync their mirrors without replaying deltas. The refactor moves the
// alphabet-switch and seed-buffer code — if one of these events goes
// missing, frontends' shadow buffers desync silently.
// ---------------------------------------------------------------------------

namespace {
struct EventLog {
    std::vector<int> events;
    std::vector<std::string> texts;
};
void record_event(int event_type, const char* text, void* user_data) {
    auto* log = static_cast<EventLog*>(user_data);
    log->events.push_back(event_type);
    log->texts.push_back(text ? text : "");
}
} // namespace

TEST_CASE("contracts/buffer-clear event fires on every buffer reset path") {
    ScopedContext ctx(800, 600);
    EventLog log;
    dasher_set_output_callback(ctx, record_event, &log);

    // 1. Explicit output reset.
    dasher_reset_output_text(ctx);
    REQUIRE(log.events.size() == 1);
    CHECK(log.events.back() == 2);
    CHECK(log.texts.back().empty());

    // 2. Full dasher_reset.
    dasher_reset(ctx);
    REQUIRE(log.events.size() == 2);
    CHECK(log.events.back() == 2);

    // 3. Alphabet change clears the buffer (documented side effect,
    //    dasher.h / CAPI dasher_set_alphabet_id).
    dasher_set_alphabet_id(ctx, "English");
    REQUIRE(log.events.size() == 3);
    CHECK(log.events.back() == 2);

    // 4. Seeding the buffer from external text fires event 2 FIRST (before
    //    any subsequent output), per the RFC 0015 contract in dasher.h.
    REQUIRE(dasher_seed_buffer(ctx, "hello world", 11) == 0);
    REQUIRE(log.events.size() == 4);
    CHECK(log.events.back() == 2);
    CHECK(std::string(dasher_get_output_text(ctx)) == "hello world");

    // No other stray events during the above.
    for (int e : log.events)
        CHECK(e == 2);

    dasher_set_output_callback(ctx, nullptr, nullptr);
}

// ---------------------------------------------------------------------------
// Engine-error lifecycle (RFC 0009 Amendment 2; todo.md 0.6, previously
// deferred). Uses the dasher_test_inject_failure hook to drive a real C++
// exception through the boundary guard and pins the full contract:
//   throw at a per-frame entry point → caught at the boundary →
//   DASHER_LOG_ERROR via the log callback → engineError latches →
//   frame/mouse/key no-op (outputs zeroed) → NOT cleared by dasher_reset →
//   cleared only by destroying and recreating the context.
// ---------------------------------------------------------------------------

namespace {
struct LogCapture {
    std::vector<int> levels;
    std::vector<std::string> messages;
};
void capture_log(int level, const char* message, void* user_data) {
    auto* log = static_cast<LogCapture*>(user_data);
    log->levels.push_back(level);
    log->messages.push_back(message ? message : "");
}
} // namespace

TEST_CASE("contracts/engine-error lifecycle: frame throw latches, no-ops, recreate clears") {
    ScopedContext ctx(800, 600);
    REQUIRE(ctx.ctx != nullptr);

    // Healthy baseline: frame produces commands, no fault.
    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);
    REQUIRE(cc >= 6);
    CHECK(dasher_has_engine_error(ctx) == 0);

    LogCapture log;
    dasher_set_log_callback(ctx, capture_log, &log, 0);

    // Inject + trigger: outputs must come back zeroed, fault latched.
    dasher_test_inject_failure(ctx, DASHER_FAIL_INJECT_FRAME);
    cmds = reinterpret_cast<int*>(0x1);
    cc = -1;
    dasher_frame(ctx, 1016, &cmds, &cc, &strs, &sc);
    CHECK(cmds == nullptr);
    CHECK(cc == 0);
    CHECK(dasher_has_engine_error(ctx) == 1);

    // The failure reached the log callback at ERROR level with the
    // entry-point context.
    REQUIRE(log.levels.size() >= 1);
    CHECK(log.levels.back() == DASHER_LOG_ERROR);
    CHECK(log.messages.back().find("dasher_frame") != std::string::npos);
    CHECK(log.messages.back().find("test-injected failure") != std::string::npos);

    // Disarmed but still latched: frame and input entry points no-op
    // (no crash, no engine mutation, outputs still zeroed).
    dasher_test_inject_failure(ctx, DASHER_FAIL_INJECT_NONE);
    dasher_frame(ctx, 1032, &cmds, &cc, &strs, &sc);
    CHECK(cc == 0);
    dasher_mouse_move(ctx, 400.0f, 300.0f);
    dasher_mouse_down(ctx);
    dasher_mouse_up(ctx);
    dasher_key_event(ctx, DASHER_KEY_PRIMARY, 1);
    dasher_key_event(ctx, DASHER_KEY_PRIMARY, 0);
    CHECK(dasher_has_engine_error(ctx) == 1);

    // dasher_reset must NOT clear the fault (documented: only recreation).
    dasher_reset(ctx);
    CHECK(dasher_has_engine_error(ctx) == 1);

    // Recreate: fresh context is healthy and frames again.
    dasher_destroy(ctx.ctx);
    ctx.ctx = dasher_create(TEST_DATA_DIR, ctx.dir.c_str(), nullptr);
    REQUIRE(ctx.ctx != nullptr);
    CHECK(dasher_has_engine_error(ctx.ctx) == 0);
    dasher_set_screen_size(ctx.ctx, 800, 600);
    cmds = nullptr;
    cc = 0;
    dasher_frame(ctx.ctx, 1048, &cmds, &cc, &strs, &sc);
    CHECK(cc >= 6);
    CHECK(dasher_has_engine_error(ctx.ctx) == 0);
}

TEST_CASE("contracts/engine-error lifecycle: failed Realize latches, retry recovers") {
    // The review-P1-#77 scenario: a failed Realize latches engineError
    // while !realized; a subsequent dasher_set_screen_size must recreate
    // the interface and clear the fault (not no-op forever).
    ScopedContext ctx; // never realized
    REQUIRE(ctx.ctx != nullptr);

    dasher_test_inject_failure(ctx, DASHER_FAIL_INJECT_REALIZE);
    dasher_set_screen_size(ctx, 800, 600);
    CHECK(dasher_has_engine_error(ctx) == 1);

    dasher_test_inject_failure(ctx, DASHER_FAIL_INJECT_NONE);
    dasher_set_screen_size(ctx, 800, 600); // retry recreates the interface
    CHECK(dasher_has_engine_error(ctx) == 0);

    int* cmds = nullptr;
    int cc = 0;
    char** strs = nullptr;
    int sc = 0;
    dasher_frame(ctx, 1000, &cmds, &cc, &strs, &sc);
    CHECK(cc >= 6);
}
