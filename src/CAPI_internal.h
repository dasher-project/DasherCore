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

#include "DasherCore/DashIntfScreenMsgs.h"
#include "DasherCore/ColorPalette.h"
#include "DasherCore/XmlSettingsStore.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// Defined in CAPI.cpp until the Phase 2.3 extraction moves them out.
class CommandScreen;
class PointerInput;

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
    std::string tlString;
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
    std::string stringBuf;

    // Buffers backing const char* returns from various getters. These
    // MUST live in dasher_ctx (not file-scope static) so that two
    // contexts don't trample each other's returned pointers — a real
    // cross-context bug noted in the codebase review (Tier 1 #4).
    std::vector<std::string> stringValues; // dasher_get_palette_name / alphabet_name / parameter_string_values
    std::string gameTextBuf;               // dasher_game_get_target_text

    // Strand 2 (RFC 0013): label strings for dasher_get_visible_nodes. Owned
    // here so the returned char** is stable until the next visible_nodes/frame
    // call, mirroring the command-buffer ownership contract.
    std::vector<std::string> nodeLabelStrings;
    std::vector<char*> nodeLabelPtrs;

    // Appearance model state (RFC 0007). Lives at the C API layer — appearance
    // is a shell/canvas concern, not a DasherCore engine parameter. Persisted to
    // <userDir>/appearance_settings.xml. The active palette (SP_COLOUR_ID) is
    // derived from these via resolveAppearance(), so an auto-switch can never
    // overwrite the user's explicit preference.
    int appearanceMode = 0;   // 0=system, 1=light, 2=dark
    int systemAppearance = 1; // transient OS input: 1=light, 2=dark
    std::string lightPalette; // user's preferred palette for light appearance
    std::string darkPalette;  // user's preferred palette for dark appearance
    bool appearanceLoaded = false;
    dasher_output_callback outputCb = nullptr;
    // Pending text measurement callback: kept here (not only on the screen)
    // because frontends register callbacks before dasher_set_screen_size
    // creates the CommandScreen; set_screen_size forwards it.
    dasher_text_size_callback textSizeCb = nullptr;
    void* textSizeCbUserData = nullptr;
    void* outputCbUserData = nullptr;
    dasher_message_callback messageCb = nullptr;
    void* messageCbUserData = nullptr;
    dasher_speak_callback speakCb = nullptr;
    void* speakCbUserData = nullptr;
    dasher_clipboard_callback clipboardCb = nullptr;
    void* clipboardCbUserData = nullptr;
    dasher_parameter_callback paramCb = nullptr;
    void* paramCbUserData = nullptr;

    // Diagnostic log callback (replaces the former CFileLogger/CBasicLog/UserLog
    // systems). When null, log messages are silently discarded.
    dasher_log_callback logCb = nullptr;
    void* logCbUserData = nullptr;
    int logCbMinLevel = 0;

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
inline unsigned long inputTime(dasher_ctx* ctx) {
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
inline void boundary_error(dasher_ctx* ctx, const char* context, const char* detail, bool latch) noexcept {
    if (latch && ctx) ctx->engineError = true;
    if (!ctx || !ctx->logCb || 3 /*ERROR*/ < ctx->logCbMinLevel) return;
    char buf[256];
    const int n = snprintf(buf, sizeof(buf), "%s: %s", context ? context : "", detail ? detail : "");
    if (n < 0) return; // encoding error — nothing useful to report
    // snprintf always null-terminates (size > 0), so buf is valid even if truncated.
    ctx->logCb(3, buf, ctx->logCbUserData);
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

#endif // DASHER_CAPI_INTERNAL_H
