#ifndef DASHER_CAPI_INTERNAL_H
#define DASHER_CAPI_INTERNAL_H

// CAPI_internal.h — shared internals for the C API implementation.
//
// NOT installed, NOT part of the public API/ABI: everything here lives
// behind the opaque dasher_ctx or is internal linkage. The public surface
// is src/dasher.h alone (todo.md Phase 2: CAPI.cpp is being split into
// internal TUs; they all include this header).
//
// Contents:
//   - struct dasher_ctx        (moved verbatim from CAPI.cpp)
//   - small shared utilities   (nowMs, lround_int, clamp_int, colorToARGB)
//   - the boundary-exception guard (Rule 4: no C++ exception crosses
//     extern "C"; replaces the per-function try/catch boilerplate)

#include "dasher.h"

#include "DasherCore/ControlManager.h"
#include "DasherCore/DashIntfScreenMsgs.h"
#include "DasherCore/ColorPalette.h"
#include "DasherCore/XmlSettingsStore.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <deque>
#include <memory>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Defined in CAPI_screen.h (Phase 2.3): the command-buffer screen and the
// pointer input device.
class CommandScreen;
class PointerInput;

// Internal-linkage intent for cross-TU helpers: keeps them out of the
// shared library's dynamic symbol table (the C API surface is dasher.h's
// DASHER_API functions, nothing else). No effect on static/direct compiles.
#ifdef _WIN32
#define DASHER_LOCAL
#elif defined(__GNUC__)
#define DASHER_LOCAL __attribute__((visibility("hidden")))
#else
#define DASHER_LOCAL
#endif

// ── Small shared utilities ─────────────────────────────────────────────────

inline unsigned long nowMs() {
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

inline int lround_int(double v) {
    int i = static_cast<int>(v);
    return (v - i >= 0.5) ? i + 1 : (v - i <= -0.5) ? i - 1 : i;
}

inline int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ── Session context ─────────────────────────────────────────────────────────

struct dasher_ctx {
    struct Interface;
    std::unique_ptr<Dasher::XmlSettingsStore> settings;
    // True when dasher_settings.xml existed in the user dir at create time.
    // Used to distinguish "no saved preference, use a sensible default"
    // from "the user deliberately saved this value" — a value-based
    // comparison can't tell them apart (the compiled-in default IS a
    // legitimate user choice).
    bool settingsFileExisted = false;
    std::unique_ptr<CommandScreen> screen;
    PointerInput* input = nullptr;
    Dasher::CDashIntfScreenMsgs* intf = nullptr;
    std::string editBuffer;
    size_t cursorPos = 0;
    // The engine's own timeline: the timestamp of the most recent
    // dasher_frame(). Input events (mouse/key) are stamped with this so the
    // engine never subtracts across clocks — frontends pass their own
    // timeline to dasher_frame (compositor frame time, uptime, anything),
    // and mixing that with a steady_clock stamp here poisoned
    // LP_FRAMERATE/slow-start at every stop/restart (the Windows + Android
    // restart drift, #60). v5 never had this because its frontends embedded
    // the engine on one clock.
    int64_t lastFrameMs = 0;
    // Last input-event stamp (see inputTime): strictly increasing across
    // consecutive inputs within one frame window, so intra-frame gesture
    // ordering/durations survive.
    int64_t lastInputMs = 0;
    // Typing rate tracker (RFC 0012): timestamps of recent character outputs.
    std::deque<std::chrono::steady_clock::time_point> rateTimestamps;
    bool realized = false;
    // Host's low-memory request, retained on the ctx so the transactional
    // realize retry can reapply it to a recreated interface (review P1 #77).
    bool lowMemory = false;
    bool mouseDown = false;
    // Latched true when a C++ exception was caught at the C-API boundary of a
    // per-frame entry point (frame/mouse/key). Once set, those entry points
    // no-op until the engine is destroyed and recreated — the engine state is
    // indeterminate after a mid-frame throw. Frontends query this via
    // dasher_has_engine_error(). Per RFC 0009 Amendment 2; not cleared by
    // dasher_reset (only by recreating the context).
    bool engineError = false;
    std::string pendingAlphabet;
    std::string dataDir;
    std::string userDir;

    // Buffers backing const char*/char** returns from various getters. These
    // MUST live in dasher_ctx (not file-scope static) so that two
    // contexts don't trample each other's returned pointers — a real
    // cross-context bug noted in the codebase review (Tier 1 #4).
    // Distinct families, distinct lifetimes: tlString is the shared scratch
    // for most string getters (see the "valid until next API call" contract
    // in dasher.h); stringBuf backs the locale getters independently;
    // gameTextBuf the game getters; stringValues the permitted-value lists;
    // nodeLabel* Strand 2 labels.
    struct StringScratch {
        std::string tlString;                      // shared scratch: most string getters
        std::string stringBuf;                     // locale / localized-string getters
        std::string gameTextBuf;                   // dasher_game_get_target_text / wrong_text
        std::vector<std::string> stringValues;     // palette / alphabet / parameter string values
        std::vector<std::string> nodeLabelStrings; // Strand 2 (RFC 0013) node labels
        std::vector<char*> nodeLabelPtrs;
    } scratch;

    // Appearance model state (RFC 0007). Lives at the C API layer — appearance
    // is a shell/canvas concern, not a DasherCore engine parameter. Persisted to
    // <userDir>/appearance_settings.xml. The active palette (SP_COLOUR_ID) is
    // derived from these via resolveAppearance(), so an auto-switch can never
    // overwrite the user's explicit preference.
    struct Appearance {
        int mode = 0;             // DASHER_APPEARANCE_MODE_SYSTEM/LIGHT/DARK
        int systemAppearance = 1; // transient OS input: DASHER_PALETTE_APPEARANCE_LIGHT/DARK
        std::string lightPalette; // user's preferred palette for light appearance
        std::string darkPalette;  // user's preferred palette for dark appearance
        bool loaded = false;
    } appearance;

    // Frontend callbacks (all optional; null = feature disabled).
    struct Callbacks {
        dasher_output_callback outputCb = nullptr;
        void* outputCbUserData = nullptr;
        // Pending text measurement callback: kept here (not only on the screen)
        // because frontends register callbacks before dasher_set_screen_size
        // creates the CommandScreen; set_screen_size forwards it.
        dasher_text_size_callback textSizeCb = nullptr;
        void* textSizeCbUserData = nullptr;
        dasher_message_callback messageCb = nullptr;
        void* messageCbUserData = nullptr;
        dasher_speak_callback speakCb = nullptr;
        void* speakCbUserData = nullptr;
        dasher_clipboard_callback clipboardCb = nullptr;
        void* clipboardCbUserData = nullptr;
        dasher_parameter_callback paramCb = nullptr;
        void* paramCbUserData = nullptr;
        // Diagnostic log callback (replaces the former
        // CFileLogger/CBasicLog/UserLog systems). When null, log messages
        // are silently discarded.
        dasher_log_callback logCb = nullptr;
        void* logCbUserData = nullptr;
        int logCbMinLevel = 0;
    } callbacks;

    // Test-only failure injection (todo.md 0.6): set via
    // dasher_test_inject_failure; capi::test_inject throws at the armed
    // entry point so tests can drive the Rule-4 boundary and the
    // engineError lifecycle end to end. Never set by production frontends;
    // zero cost when disarmed (one integer compare per guarded body).
    int failInjectSite = DASHER_FAIL_INJECT_NONE;

    struct CustomActionEntry {
        std::string name;
        dasher_action_callback callback;
        void* userData;
    };
    std::vector<CustomActionEntry> customActions;

    // Defined out-of-line in CAPI.cpp (struct dasher_ctx::Interface): it
    // carries the full engine-side edit-buffer/callback overrides.
};

// ── Colour helper ───────────────────────────────────────────────────────────

inline int32_t colorToARGB(const Dasher::ColorPalette::Color& c) {
    int a = c.Alpha, r = c.Red, g = c.Green, b = c.Blue;

    if (a >= 0 && a <= 1 && r >= 0 && r <= 1 && g >= 0 && g <= 1 && b >= 0 && b <= 1) {
        a = a * 255;
        r = r * 255;
        g = g * 255;
        b = b * 255;
    }

    a = clamp_int(a, 0, 255);
    r = clamp_int(r, 0, 255);
    g = clamp_int(g, 0, 255);
    b = clamp_int(b, 0, 255);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

// ── Input event stamping ────────────────────────────────────────────────────

// Stamp for input events arriving between frames: the engine's own timeline
// (the most recent dasher_frame time), never steady_clock — the engine
// subtracts input stamps from frame times, so mixing clocks corrupts
// framerate/slow-start state at every stop/restart (#60). Multiple inputs
// within one frame window get strictly increasing stamps so gesture timing
// (stylus tap vs hold, multi-press) still sees distinct timestamps — but
// capped 2ms past the frame time, so a burst of same-frame inputs can never
// outrun the next frame stamp and underflow the unsigned elapsed-time math
// downstream (slow-start).
inline DASHER_LOCAL unsigned long inputTime(dasher_ctx* ctx) {
    if (ctx->lastInputMs < ctx->lastFrameMs)
        ctx->lastInputMs = ctx->lastFrameMs;
    else if (ctx->lastInputMs < ctx->lastFrameMs + 2)
        ctx->lastInputMs += 1;
    return static_cast<unsigned long>(ctx->lastInputMs);
}

// ── C API Boundary Exception Guard ──────────────────────────────────────────
//
// Enforces Rule 4: never throw across the C API boundary. All exceptions are
// caught and reported via the log callback at level 3 (ERROR) when registered
// and at/above min_level; otherwise silently discarded.
//
// noexcept and allocation-free (fixed buffer + snprintf). A catch handler that
// itself throws — e.g. a std::string concat hitting bad_alloc — re-violates the
// boundary and, for void setters, cannot recover. RFC 0009 Amendment 2 requires
// this property so engine fault context reliably reaches the frontend ring
// buffer before the function returns.

namespace capi {

// Logs the failure (when a callback is registered) and optionally latches the
// engine fault flag. Context strings are stable literals from call sites.
//
// Ordering note: the flag latches BEFORE the log callback fires (the old
// per-site handlers logged first). Only observable to a re-entrant callback
// that queries dasher_has_engine_error() mid-log — and latch-first is the
// safer order: the fault is recorded even if the callback itself throws.
DASHER_LOCAL inline void boundary_error(dasher_ctx* ctx, const char* context, const char* detail, bool latch) noexcept {
    if (latch && ctx) ctx->engineError = true;
    if (!ctx || !ctx->callbacks.logCb || 3 /*ERROR*/ < ctx->callbacks.logCbMinLevel) return;
    char buf[256];
    const int n = snprintf(buf, sizeof(buf), "%s: %s", context ? context : "", detail ? detail : "");
    if (n < 0) return; // encoding error — nothing useful to report
    // snprintf always null-terminates (size > 0), so buf is valid even if truncated.
    ctx->callbacks.logCb(3, buf, ctx->callbacks.logCbUserData);
}

// Void-bodied guard. latch selects the per-frame entry-point behaviour
// (frame/mouse/key latch engineError; setters do not).
//
//   capi::guarded(ctx, "dasher_set_bool_parameter", /*latch=*/false, [&] {
//       ctx->intf->SetBoolParameter(key, value != 0);
//   });
template <typename Body>
inline void guarded(dasher_ctx* ctx, const char* context, bool latch, Body&& body) {
    try {
        body();
    } catch (const std::exception& e) {
        boundary_error(ctx, context, e.what(), latch);
    } catch (...) {
        boundary_error(ctx, context, "unknown exception", latch);
    }
}

// Value-returning guard with the caller's error result. No value-returning
// entry point latches the fault flag today; add a latch parameter here if
// one ever needs to.
//
//   return capi::guarded_result(ctx, "dasher_set_offset", -1, [&]() -> int {
//       ...; return 0;
//   });
template <typename Result, typename Body>
inline Result guarded_result(dasher_ctx* ctx, const char* context, Result error_result, Body&& body) {
    // Guard against the classic trap: guarded_result(ctx, c, 0, [&]() -> long {...})
    // would deduce Result=int and silently truncate a long success value.
    static_assert(std::is_convertible_v<std::invoke_result_t<Body&>, Result>,
                  "guard error_result type must match the body's return type");
    try {
        return body();
    } catch (const std::exception& e) {
        boundary_error(ctx, context, e.what(), /*latch=*/false);
        return error_result;
    } catch (...) {
        boundary_error(ctx, context, "unknown exception", /*latch=*/false);
        return error_result;
    }
}

// Test-only: throws when the context's armed injection site matches. Called
// as the first line of guarded bodies (see dasher_test_inject_failure).
inline void test_inject(const dasher_ctx* ctx, int site) {
    if (ctx->failInjectSite == site) throw std::runtime_error("test-injected failure");
}

} // namespace capi

// ── Appearance model (implemented in CAPI_appearance.cpp; RFC 0007) ────────
//
// Shared with CAPI.cpp: dasher_create seeds + resolves the model at
// startup; dasher_save_settings persists the sidecar alongside settings.
namespace capi {
// Recompute the active palette from mode + system + preferences and write
// it to SP_COLOUR_ID (what the canvas renders). Never clobbers the user's
// persisted preference.
DASHER_LOCAL void resolveAppearance(dasher_ctx* ctx);
// Sidecar persistence (<userDir>/appearance_settings.xml). Non-fatal on
// any error; load is once-only (ctx->appearance.loaded).
DASHER_LOCAL void loadAppearanceSettings(dasher_ctx* ctx);
DASHER_LOCAL void saveAppearanceSettings(dasher_ctx* ctx);
} // namespace capi

// ── Locale tables (implemented in CAPI_locale.cpp) ─────────────────────────
//
// Process-global by design (see CAPI_locale.cpp's header comment and
// todo.md 5.2). Read by parameter introspection for localized names, so
// they cross the TU boundary via accessors rather than externs.
namespace capi {
DASHER_LOCAL const std::unordered_map<std::string, std::string>& localeStrings();
DASHER_LOCAL const std::unordered_map<std::string, std::string>& overrideStrings();
} // namespace capi

// ── Edit-buffer helpers (implemented in CAPI_edit.cpp) ─────────────────────

// Word/sentence/paragraph/char range walker over a UTF-8 buffer — the
// engine's ctrlMove/ctrlDelete/GetTextAroundCursor arithmetic. Shared with
// the Interface overrides in CAPI.cpp.
namespace capi {
DASHER_LOCAL void getRange(const std::string& buf, bool bForwards, Dasher::EditDistance dist, size_t& ioStart,
                           size_t& ioEnd);
} // namespace capi

// Notify subscribers that the edit buffer was cleared wholesale (event
// DASHER_EVENT_BUFFER_CLEAR). Insert/delete deltas can't express this;
// without the event every frontend must know which API calls clear the
// buffer and re-sync manually (Dasher-GTK's stale output pane after "New"
// was exactly this bug). Fires from CAPI.cpp's reset paths and
// CAPI_edit.cpp's seed_buffer.
inline DASHER_LOCAL void notify_buffer_cleared(dasher_ctx* ctx) {
    if (ctx->callbacks.outputCb)
        ctx->callbacks.outputCb(DASHER_EVENT_BUFFER_CLEAR, "", ctx->callbacks.outputCbUserData);
}

// ── Custom-action adapter ───────────────────────────────────────────────────

// Adapt a C-API action callback to the engine's C++ custom-action callback:
// marshals the attribute map into the parallel key/value char* arrays the C
// signature needs. One definition for both registration paths — the
// Interface's GetPendingCustomActions (actions registered before the
// control manager exists) and dasher_register_action's direct registration
// into a live manager — which previously carried identical copies of this
// lambda (todo.md 3.1).
namespace capi {
inline DASHER_LOCAL Dasher::CustomActionCallback make_custom_action_adapter(dasher_action_callback cb, void* ud) {
    return [cb, ud](const std::string& name, const std::map<std::string, std::string>& attrs) {
        if (!cb) return;
        std::vector<std::string> keys, values;
        for (const auto& [k, v] : attrs) {
            keys.push_back(k);
            values.push_back(v);
        }
        std::vector<const char*> keyPtrs, valPtrs;
        keyPtrs.reserve(keys.size());
        valPtrs.reserve(values.size());
        for (auto& k : keys)
            keyPtrs.push_back(k.c_str());
        for (auto& v : values)
            valPtrs.push_back(v.c_str());
        cb(name.c_str(), static_cast<int>(keyPtrs.size()), keyPtrs.data(), valPtrs.data(), ud);
    };
}
} // namespace capi

#endif // DASHER_CAPI_INTERNAL_H
