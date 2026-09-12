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
// crashing) and the locale semantics above (per-context since v3, with the
// ctx-less introspection snapshot).

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
    CHECK(std::string(dasher_find_companion_palette(nullptr, "Default")) == "");
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
// String sentinels. "" is the universal failure value since CAPI version 2
// (todo.md Phase 4 unified the outliers). The single remaining NULL return
// is dasher_get_localized_string: "missing translation" is a distinct state
// from an empty translation, and frontends use it to pick fallbacks.
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
    // Invalid id sentinels: "" since CAPI version 2 (was "Unknown").
    CHECK(std::string(dasher_get_language_model_name(-999)) == "");
    CHECK(std::string(dasher_get_language_model_description(-999)) == "");
    CHECK(std::string(dasher_get_parameter_enum_name(-999, 0)) == "");

    ScopedContext ctx(800, 600);
    CHECK(std::string(dasher_get_palette_name(ctx, -5)) == "");
    CHECK(std::string(dasher_get_palette_name(ctx, 1 << 20)) == "");
    CHECK(std::string(dasher_get_alphabet_name(ctx, 1 << 20)) == "");

    // NULL sentinels (the outliers — see file header).
    CHECK(std::string(dasher_find_companion_palette(ctx, "No Such Palette Exists")) == "");
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
// Locale semantics (CAPI v3): the four locale functions are per-context;
// the ctx-less dasher_get_parameter_info reads a process-global snapshot
// updated by the most recent set_locale / set_string_override ("last
// context wins" — an ABI constraint, documented in dasher.h).
// ---------------------------------------------------------------------------

TEST_CASE("contracts/locale is per-context (CAPI v3)") {
    // Was process-global (pinned here as a wart in earlier versions); v3
    // made the four locale functions operate on the calling ctx only.
    ScopedContext a(800, 600);
    ScopedContext b(800, 600);

    // Default state on both.
    CHECK(std::string(dasher_get_locale(a)) == "en");
    CHECK(std::string(dasher_get_locale(b)) == "en");

    // strings_de.json ships in Strings/ — setting on A must succeed and
    // affect ONLY A. B stays English.
    REQUIRE(dasher_set_locale(a, "de") == 0);
    CHECK(std::string(dasher_get_locale(a)) == "de");
    CHECK(std::string(dasher_get_locale(b)) == "en");
    CHECK(dasher_get_localized_string(b, "BP_DRAW_MOUSE_LINE.label") == nullptr);

    // Translations resolve on the context that loaded them.
    const char* de_label = dasher_get_localized_string(a, "BP_DRAW_MOUSE_LINE.label");
    CHECK(de_label != nullptr);

    // Overrides are equally per-context.
    dasher_set_string_override(a, "test.contracts.override", "viaA");
    CHECK(std::string(dasher_get_localized_string(a, "test.contracts.override")) == "viaA");
    CHECK(dasher_get_localized_string(b, "test.contracts.override") == nullptr);
    dasher_set_string_override(a, "test.contracts.override", nullptr); // clear

    // Unknown locale: refused, previous locale retained on A.
    CHECK(dasher_set_locale(a, "zz-nonexistent") == -1);
    CHECK(std::string(dasher_get_locale(a)) == "de");

    // Reset on A leaves A English.
    REQUIRE(dasher_set_locale(a, "en") == 0);
    CHECK(std::string(dasher_get_locale(a)) == "en");
    CHECK(std::string(dasher_get_locale(b)) == "en");
}

TEST_CASE("contracts/locale: introspection snapshot is last-context-wins across contexts") {
    // Two live contexts, different locales: the ctx-less introspection
    // reflects whichever set_locale ran LAST (an ABI constraint —
    // dasher_get_parameter_info takes no ctx). Overrides union across
    // contexts per key and persist until cleared (documented in
    // CAPI_locale.cpp); pinned here so the trap is visible, not latent.
    ScopedContext a(800, 600);
    ScopedContext b(800, 600);

    auto first_label = [&]() {
        for (int i = 0; i < dasher_get_parameter_count(); i++) {
            dasher_parameter_info info{};
            if (dasher_get_parameter_info(i, &info) == 0 && info.name[0] != '\0') return std::string(info.name);
        }
        return std::string();
    };

    REQUIRE(dasher_set_locale(b, "de") == 0); // B last -> German
    const std::string via_b = first_label();
    REQUIRE(dasher_set_locale(a, "fr") == 0); // A last -> French
    const std::string via_a = first_label();
    CHECK(via_a != via_b); // snapshot followed the most recent setter

    // Overrides union per key (across contexts) and survive the writing
    // context's death — the snapshot is process state, documented in
    // CAPI_locale.cpp. Pinned so the trap is visible, not latent.
    dasher_set_string_override(b, "BP_DRAW_MOUSE_LINE.label", "override-via-b");
    auto snapshot_has_override = [&]() {
        dasher_parameter_info info{};
        for (int i = 0; i < dasher_get_parameter_count(); i++) {
            if (dasher_get_parameter_info(i, &info) != 0) continue;
            if (std::string(info.name) == "override-via-b") return true;
        }
        return false;
    };
    CHECK(snapshot_has_override());                               // visible through the ctx-less path
    dasher_destroy(b.ctx);                                        // writer gone...
    CHECK(snapshot_has_override());                               // ...snapshot keeps the override
    b.ctx = dasher_create(TEST_DATA_DIR, b.dir.c_str(), nullptr); // fresh B
    // Clean up the union entry for later cases in this binary.
    ScopedContext cleaner;
    dasher_set_string_override(cleaner, "BP_DRAW_MOUSE_LINE.label", nullptr);
    dasher_set_locale(cleaner, "en");
}

TEST_CASE("contracts/locale: ctx-less introspection follows the most recent locale") {
    // ABI constraint, documented in dasher.h: dasher_get_parameter_info
    // takes no ctx, so its localized names read a process-global snapshot
    // updated by the most recent set_locale ("last context wins").
    ScopedContext a(800, 600);

    // Find a parameter whose German label differs from the English one.
    auto label_of = [&](int index) {
        dasher_parameter_info info{};
        REQUIRE(dasher_get_parameter_info(index, &info) == 0);
        return std::string(info.name);
    };
    int differing = -1;
    const int count = dasher_get_parameter_count();
    std::string en_label;
    for (int i = 0; i < count && differing < 0; i++) {
        en_label = label_of(i);
        if (!en_label.empty() && en_label.find(' ') != std::string::npos) differing = i; // heuristic
    }
    REQUIRE(differing >= 0);

    // Set German on the (only) context: the snapshot updates, so the
    // ctx-less introspection now returns the localized name.
    REQUIRE(dasher_set_locale(a, "de") == 0);
    dasher_parameter_info info{};
    REQUIRE(dasher_get_parameter_info(differing, &info) == 0);
    // German differs from English for the sampled label (translations
    // exist per test_locale_files' corpus guard).
    CHECK(std::string(info.name) != en_label);

    REQUIRE(dasher_set_locale(a, "en") == 0);
    REQUIRE(dasher_get_parameter_info(differing, &info) == 0);
    CHECK(std::string(info.name) == en_label);
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

// ---------------------------------------------------------------------------
// Permitted-value cache (todo.md 4.3). The indexed list getters and
// dasher_get_parameter_string_values share one memoized list per ctx,
// invalidated by ANY parameter change. Pins: stable iteration, cache
// coherence across the getter family, and invalidation after a real
// parameter change (palette switch fires OnParameterChanged).
// ---------------------------------------------------------------------------

TEST_CASE("contracts/permitted-value cache: iteration stable, family coherent, invalidates on change") {
    ScopedContext ctx(800, 600);

    const int count = dasher_get_alphabet_count(ctx);
    REQUIRE(count > 1);

    // Full iteration, twice: the second pass must see identical values
    // (cache refill must not corrupt or reorder), and the names must match
    // a get_parameter_string_values snapshot taken through the same cache.
    std::vector<std::string> first_pass;
    for (int i = 0; i < count; i++)
        first_pass.emplace_back(dasher_get_alphabet_name(ctx, i));
    for (int i = 0; i < count; i++)
        CHECK(std::string(dasher_get_alphabet_name(ctx, i)) == first_pass[i]);

    std::vector<const char*> snapshot(count);
    REQUIRE(dasher_get_parameter_string_values(ctx, dasher_find_parameter_key("SP_ALPHABET_ID"), snapshot.data(),
                                               count) == count);
    for (int i = 0; i < count; i++)
        CHECK(std::string(snapshot[i]) == first_pass[i]);

    // The active alphabet appears in the list.
    const std::string active = dasher_get_alphabet_id(ctx);
    bool found = false;
    for (auto& n : first_pass)
        found |= (n == active);
    CHECK(found);

    // A parameter change (palette switch fires OnParameterChanged) must
    // invalidate, not corrupt: the alphabet list is still complete and
    // correct afterwards, and the palette list reflects the new palette.
    const std::string old_palette = dasher_get_current_palette(ctx);
    std::string target;
    const int pcount = dasher_get_palette_count(ctx);
    for (int i = 0; i < pcount; i++) {
        std::string n = dasher_get_palette_name(ctx, i);
        if (n != old_palette) {
            target = n;
            break;
        }
    }
    REQUIRE(!target.empty());
    dasher_set_palette(ctx, target.c_str());
    CHECK(std::string(dasher_get_current_palette(ctx)) == target);

    // Post-change: alphabet list unchanged in content...
    CHECK(dasher_get_alphabet_count(ctx) == count);
    for (int i = 0; i < count; i++)
        CHECK(std::string(dasher_get_alphabet_name(ctx, i)) == first_pass[i]);
    // ...and the palette list still contains the newly active palette.
    bool has_new = false;
    for (int i = 0; i < dasher_get_palette_count(ctx); i++)
        has_new |= (std::string(dasher_get_palette_name(ctx, i)) == target);
    CHECK(has_new);
}

TEST_CASE("contracts/permitted-value cache: realize boundaries invalidate") {
    // Realize() populates the alphabet/colour/filter lists WITHOUT firing
    // OnParameterChanged, so a frontend that builds its pickers before
    // dasher_set_screen_size would cache the empty pre-realize answers
    // forever if the realize boundary didn't invalidate. (Found by review
    // loop 1; this test is the regression guard.)
    ScopedContext unrealized; // created, never given a screen size
    REQUIRE(unrealized.ctx != nullptr);
    const int pre = dasher_get_alphabet_count(unrealized); // caches the answer
    dasher_set_screen_size(unrealized, 800, 600);          // realize
    const int post = dasher_get_alphabet_count(unrealized);
    CHECK(post > pre);
    REQUIRE(post > 1); // Data/ ships hundreds; a stale 0 or 1 fails here

    // Same boundary on the failed-Realize retry path: the cache lives on
    // the ctx and must not survive the interface recreation either.
    ScopedContext retry;
    dasher_test_inject_failure(retry, DASHER_FAIL_INJECT_REALIZE);
    dasher_set_screen_size(retry, 800, 600); // realize fails, engineError latches
    CHECK(dasher_has_engine_error(retry) == 1);
    (void)dasher_get_alphabet_count(retry); // caches whatever the broken state reports
    dasher_test_inject_failure(retry, DASHER_FAIL_INJECT_NONE);
    dasher_set_screen_size(retry, 800, 600); // retry: interface recreated + realized
    CHECK(dasher_has_engine_error(retry) == 0);
    // Same data dir -> same alphabet count (exact equality is deliberate).
    CHECK(dasher_get_alphabet_count(retry) == post);
}

TEST_CASE("contracts/permitted-value cache: invalid key never returns a stale list") {
    // Greptile PR #91: invalidatePermittedCache used key = -1 as its
    // sentinel, which collides with dasher_find_parameter_key's -1 for a
    // failed lookup — a query with an invalid key could receive the
    // previous parameter's cached list. The cache now carries an explicit
    // validity flag.
    ScopedContext ctx(800, 600);
    const int filter_key = dasher_find_parameter_key("SP_INPUT_FILTER");
    REQUIRE(filter_key >= 0);

    // Fill the cache with a real list, then query with keys that fail
    // lookup: must return 0, never the stale cached list.
    REQUIRE(dasher_get_parameter_string_values(ctx, filter_key, nullptr, 0) > 1);
    CHECK(dasher_get_parameter_string_values(ctx, -1, nullptr, 0) == 0);
    CHECK(dasher_get_parameter_string_values(ctx, 99999, nullptr, 0) == 0);

    // Same after a realize-boundary invalidation on a second context.
    ScopedContext fresh;
    dasher_set_screen_size(fresh, 800, 600);
    REQUIRE(dasher_get_parameter_string_values(fresh, filter_key, nullptr, 0) > 1);
    ScopedContext fresh2;
    dasher_set_screen_size(fresh2, 800, 600);
    CHECK(dasher_get_parameter_string_values(fresh2, -1, nullptr, 0) == 0);
}

namespace {
// Re-entrant probe for the param-callback test: doctest is single-threaded,
// a file-scope ctx pointer is the simplest way for the callback to reach it.
dasher_ctx* g_probeCtx = nullptr;
int g_probeKey = -1;
int g_probeCount = -1;
bool g_probeFired = false;
void probe_param_cb(int, void*) {
    if (g_probeFired) return;
    g_probeFired = true;
    g_probeCount = dasher_get_parameter_string_values(g_probeCtx, g_probeKey, nullptr, 0);
}
} // namespace

TEST_CASE("contracts/permitted-value cache: re-entrant query inside param callback sees fresh data") {
    // Greptile PR #91: the generation bump used to run after the frontend
    // callback, so a settings UI re-querying a permitted list from inside
    // the parameter-change notification read the stale cached value. The
    // cache is invalidated BEFORE the callback fires; the generation bump
    // AFTER it flags any mid-callback refill stale for the next query.
    ScopedContext ctx(800, 600);
    const int filter_key = dasher_find_parameter_key("SP_INPUT_FILTER");
    REQUIRE(filter_key >= 0);
    const int primed = dasher_get_parameter_string_values(ctx, filter_key, nullptr, 0);
    REQUIRE(primed > 1); // cache primed with the filter list

    g_probeCtx = ctx.ctx;
    g_probeKey = filter_key;
    g_probeCount = -1;
    g_probeFired = false;
    dasher_set_parameter_callback(ctx, probe_param_cb, nullptr);

    // Any parameter change fires the notification; the re-entrant query
    // inside it must see a full fresh list (the invalid key -1 scenario
    // aside, a stale cache would still have returned the same filter list,
    // so the real assertion is: fresh refill works and nothing crashes or
    // corrupts — count during callback equals the primed count).
    dasher_set_speed_percent(ctx, 250);
    REQUIRE(g_probeFired);
    CHECK(g_probeCount == primed);

    // And after the broadcast settles, the post-callback generation bump
    // made even the mid-callback refill stale — the next query still
    // returns the correct full list.
    CHECK(dasher_get_parameter_string_values(ctx, filter_key, nullptr, 0) == primed);

    dasher_set_parameter_callback(ctx, nullptr, nullptr);
    g_probeCtx = nullptr;
}

TEST_CASE("contracts/permitted-value cache: low-memory filter list is honored") {
    // Low-memory mode silently shrinks the registered input filters during
    // CreateModules — a parameter-change-silent list mutation the
    // realize-boundary invalidation must cover (with a persisted settings
    // file, realize fires no parameter change, so the boundary call is the
    // ONLY guard). The pre-realize query fills the cache; without
    // invalidation the post-realize query would return the stale cached
    // 0 and the low_count >= 1 check below would fail. Inequalities, not
    // exact counts, so module additions don't break the pin.
    ScopedContext normal(800, 600);
    const int filter_key = dasher_find_parameter_key("SP_INPUT_FILTER");
    REQUIRE(filter_key >= 0);
    const int normal_count = dasher_get_parameter_string_values(normal, filter_key, nullptr, 0);
    REQUIRE(normal_count > 1);

    ScopedContext lowmem; // low-memory BEFORE the realize
    dasher_set_low_memory_mode(lowmem, 1);
    // Fill the cache with the pre-realize answer first.
    (void)dasher_get_parameter_string_values(lowmem, filter_key, nullptr, 0);
    dasher_set_screen_size(lowmem, 800, 600); // realize: list shrinks silently
    const int low_count = dasher_get_parameter_string_values(lowmem, filter_key, nullptr, 0);
    CHECK(low_count >= 1); // > 0 proves the cache was invalidated, not stale
    CHECK(low_count < normal_count);
}
