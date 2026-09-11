#include "dasher.h"
#include "CAPI_internal.h"
#include "CAPI_screen.h"

#include "DasherCore/DashIntfScreenMsgs.h"
#include "DasherCore/DasherInput.h"
#include "DasherCore/DasherScreen.h"
#include "DasherCore/DasherInterfaceBase.h"
#include "DasherCore/ModuleManager.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/XmlSettingsStore.h"
#include "DasherCore/FileUtils.h"
#include "DasherCore/ColorPalette.h"
#include "DasherCore/ColorIO.h"
#include "DasherCore/GameModule.h"
#include "DasherCore/Alphabet/AlphInfo.h"
#include "DasherCore/Alphabet/AlphIO.h"
#include "DasherCore/DasherModel.h"
#include "DasherCore/DasherNode.h"
#include "DasherCore/DasherView.h"
#include "DasherCore/NodeCreationManager.h"
#include "DasherCore/ControlManager.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <locale.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// lround_int / clamp_int / colorToARGB / nowMs / inputTime live in
// CAPI_internal.h.
//
// Boundary-exception policy: uniform log(+latch) sites use
// capi::guarded / capi::guarded_result from CAPI_internal.h. Functions with
// bespoke failure handling keep explicit try/catch: dasher_create (error
// string + cleanup), dasher_destroy (raw log messages), and the
// deliberately-silent catches (set_visible_nodes_enabled,
// get_visible_nodes, get_viewport, import_training_text,
// get_training_path) which return a sentinel without logging — preserved
// as-is so this refactor is behaviour-identical. CAPI_edit.cpp owns its
// own silent catch (ensure_realized_for_context).

// ── Session context: Interface ─────────────────────────────────────────────
//
// struct dasher_ctx itself lives in CAPI_internal.h. Its engine-side
// behaviour overrides are defined here, out-of-line.

struct dasher_ctx::Interface : public Dasher::CDashIntfScreenMsgs {
        Interface(Dasher::CSettingsStore* s, dasher_ctx* owner) : CDashIntfScreenMsgs(s), m_owner(owner) {
            s->OnParameterChanged.Subscribe(m_owner, [this](Dasher::Parameter param) {
                if (m_owner->callbacks.paramCb) m_owner->callbacks.paramCb(static_cast<int>(param), m_owner->callbacks.paramCbUserData);
            });
        }
        ~Interface() { m_pSettingsStore->OnParameterChanged.Unsubscribe(m_owner); }
        void CreateModules() override {
            CDashIntfScreenMsgs::CreateModules();
            auto inp = std::make_unique<PointerInput>();
            m_owner->input = inp.get();
            GetModuleManager()->RegisterInputDeviceModule(std::move(inp), true);
        }

        void Message(const std::string& strText, bool bInterrupt) override {
            // Route user-facing messages to the message callback (for UI display)
            if (m_owner->callbacks.messageCb && !strText.empty())
                m_owner->callbacks.messageCb(bInterrupt ? 1 : 0, strText.c_str(), m_owner->callbacks.messageCbUserData);
            if (!m_owner->callbacks.messageCb) CDashIntfScreenMsgs::Message(strText, bInterrupt);

            // Also route to the log callback for diagnostic logging.
            // Modal/interrupt messages are WARN level; async are INFO.
            // This ensures frontends that registered dasher_set_log_callback
            // receive engine messages even if they didn't register the
            // message callback separately.
            if (m_owner->callbacks.logCb && !strText.empty()) {
                int level = bInterrupt ? 2 /*WARN*/ : 1 /*INFO*/;
                if (level >= m_owner->callbacks.logCbMinLevel) m_owner->callbacks.logCb(level, strText.c_str(), m_owner->callbacks.logCbUserData);
            }
        }

        unsigned int ctrlOffsetAfterMove(unsigned int offsetBefore, bool bForwards,
                                         Dasher::EditDistance dist) override {
            // offsetBefore is the model's node offset, which can exceed editBuffer.size()
            // because control nodes have offsets but produce no edit buffer characters.
            // Clamp to buffer bounds before using as an index.
            size_t bufLen = m_owner->editBuffer.size();
            size_t clamped = std::min(static_cast<size_t>(offsetBefore), bufLen);
            size_t start = clamped, end = clamped;
            capi::getRange(m_owner->editBuffer, bForwards, dist, start, end);
            return static_cast<unsigned int>(bForwards ? end : start);
        }

        unsigned int ctrlMove(bool bForwards, Dasher::EditDistance dist) override {
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            capi::getRange(m_owner->editBuffer, bForwards, dist, start, end);
            m_owner->cursorPos = bForwards ? end : start;
            return static_cast<unsigned int>(m_owner->cursorPos);
        }

        unsigned int ctrlDelete(bool bForwards, Dasher::EditDistance dist) override {
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            capi::getRange(m_owner->editBuffer, bForwards, dist, start, end);
            if (start == end) {
                return static_cast<unsigned int>(m_owner->cursorPos);
            }

            const auto len = static_cast<size_t>(std::abs(static_cast<long>(end) - static_cast<long>(start)));
            const auto pos = std::min(start, end);
            const std::string deleted = m_owner->editBuffer.substr(pos, len);
            m_owner->editBuffer.erase(pos, len);
            m_owner->cursorPos = pos;

            if (m_owner->callbacks.outputCb && !deleted.empty()) {
                m_owner->callbacks.outputCb(1, deleted.c_str(), m_owner->callbacks.outputCbUserData);
            }

            return static_cast<unsigned int>(m_owner->cursorPos);
        }
        void editOutput(const std::string& strText, Dasher::CDasherNode* pCause) override {
            if (m_owner->cursorPos > m_owner->editBuffer.size()) {
                m_owner->cursorPos = m_owner->editBuffer.size();
            }
            m_owner->editBuffer.insert(m_owner->cursorPos, strText);
            m_owner->cursorPos += strText.size();
            if (m_owner->callbacks.outputCb && !strText.empty()) m_owner->callbacks.outputCb(0, strText.c_str(), m_owner->callbacks.outputCbUserData);
            m_owner->rateTimestamps.push_back(std::chrono::steady_clock::now());
            CDashIntfScreenMsgs::editOutput(strText, pCause);
        }
        void editDelete(const std::string& strText, Dasher::CDasherNode* pCause) override {
            if (!strText.empty() && m_owner->editBuffer.size() >= strText.size() &&
                m_owner->cursorPos >= strText.size()) {
                m_owner->cursorPos -= strText.size();
                m_owner->editBuffer.erase(m_owner->cursorPos, strText.size());
            }
            if (m_owner->callbacks.outputCb && !strText.empty()) m_owner->callbacks.outputCb(1, strText.c_str(), m_owner->callbacks.outputCbUserData);
            CDashIntfScreenMsgs::editDelete(strText, pCause);
        }
        std::string GetContext(unsigned int start, unsigned int len) override {
            if (start >= m_owner->editBuffer.size()) return {};
            return m_owner->editBuffer.substr(start, len);
        }
        std::string GetAllContext() override { return m_owner->editBuffer; }
        int GetAllContextLenght() override { return static_cast<int>(m_owner->editBuffer.size()); }

        bool SupportsSpeech() override { return m_owner->callbacks.speakCb != nullptr; }

        void Speak(const std::string& text, bool bInterrupt) override {
            if (m_owner->callbacks.speakCb && !text.empty())
                m_owner->callbacks.speakCb(text.c_str(), bInterrupt ? 1 : 0, m_owner->callbacks.speakCbUserData);
        }

        bool SupportsClipboard() override { return m_owner->callbacks.clipboardCb != nullptr; }

        void CopyToClipboard(const std::string& text) override {
            if (m_owner->callbacks.clipboardCb && !text.empty()) {
                m_owner->callbacks.clipboardCb(text.c_str(), m_owner->callbacks.clipboardCbUserData);
            }
        }

        std::string GetTextAroundCursor(Dasher::EditDistance dist) override {
            const std::string& buf = m_owner->editBuffer;
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            // Find the extent of text around cursor: forward then backward
            capi::getRange(buf, true, dist, start, end);
            start = m_owner->cursorPos;
            capi::getRange(buf, false, dist, start, end);
            return buf.substr(start, end > start ? end - start : 0);
        }

        dasher_ctx* m_owner;

        std::vector<std::pair<std::string, Dasher::CustomActionCallback>> GetPendingCustomActions() override {
            std::vector<std::pair<std::string, Dasher::CustomActionCallback>> result;
            for (auto& entry : m_owner->customActions) {
                auto cb = entry.callback;
                auto ud = entry.userData;
                result.emplace_back(
                    entry.name, [cb, ud](const std::string& name, const std::map<std::string, std::string>& attrs) {
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
                    });
            }
            return result;
        }
};

// inputTime and the boundary-exception guard live in CAPI_internal.h.

// ── C API implementation ──────────────────────────────────────────────────


extern "C" {

static std::string s_errorString;

DASHER_API dasher_ctx* dasher_create(const char* data_dir, const char* user_dir, char** out_error) {
    if (out_error) *out_error = nullptr;
    if (!data_dir) {
        s_errorString = "data_dir is NULL";
        if (out_error) *out_error = s_errorString.data();
        return nullptr;
    }
#ifdef _WIN32
    setlocale(LC_CTYPE, ".UTF8");
#endif

    auto* ctx = new dasher_ctx();
    std::string dir(data_dir);
    ctx->dataDir = dir;
    ctx->userDir = user_dir ? std::string(user_dir) : dir;
    std::string writableDir = user_dir ? std::string(user_dir) : dir;

    // Keep the bundled data directory (read-only corpora) and the
    // user-writable directory (logs, training deltas, settings)
    // distinct so we never leak library files into CWD. Closes the
    // dasher.log and training_english_GB.txt CWD leaks (Tier 1 #5).
    Dasher::FileUtils::SetDataDirectory(dir);
    Dasher::FileUtils::SetUserDataDirectory(writableDir);

    std::string settingsPath = writableDir;
#ifdef _WIN32
    settingsPath += "\\dasher_settings.xml";
#else
    settingsPath += "/dasher_settings.xml";
#endif

    try {
        ctx->settingsFileExisted = std::filesystem::exists(settingsPath);
        ctx->settings = std::make_unique<Dasher::XmlSettingsStore>(settingsPath, nullptr);
        ctx->settings->Load();
        ctx->intf = new dasher_ctx::Interface(ctx->settings.get(), ctx);
        // Per-context user dir: this interface's training appends must
        // resolve against ITS OWN user dir even if another context is
        // created later (the FileUtils globals are process-wide,
        // last-create-wins — see CDasherInterfaceBase::WriteTrainFile).
        ctx->intf->SetUserDataDirectory(writableDir);
        // Per-context data dir, same rationale: the startup training scan
        // reads THIS context's bundled corpus, not the global's (#84).
        ctx->intf->SetDataDirectory(dir);
    } catch (const std::exception& e) {
        s_errorString = std::string("Failed to create Dasher session: ") + e.what();
        if (out_error) *out_error = s_errorString.data();
        delete ctx->intf;
        delete ctx;
        return nullptr;
    } catch (...) {
        s_errorString = "Failed to create Dasher session: unknown error";
        if (out_error) *out_error = s_errorString.data();
        delete ctx->intf;
        delete ctx;
        return nullptr;
    }

    // Load persisted appearance preferences and resolve the active palette, so
    // the user's explicit choice is reflected immediately on startup (RFC 0007).
    capi::loadAppearanceSettings(ctx);
    capi::resolveAppearance(ctx);

    return ctx;
}

DASHER_API void dasher_destroy(dasher_ctx* ctx) {
    if (!ctx) return;
    // Flush pending adaptive-training text before teardown - unflushed
    // learning used to be silently lost on the CAPI path (the interface
    // header documents frontends like iPhone flushing on background for
    // exactly this reason). Resolves against the per-context user dir.
    // A flush failure must never block destruction: surface it through the
    // diagnostic log callback if one is registered, then proceed.
    try {
        if (ctx->intf) ctx->intf->WriteTrainFileFull();
    } catch (const std::exception& e) {
        if (ctx->callbacks.logCb && 3 /*ERROR*/ >= ctx->callbacks.logCbMinLevel) ctx->callbacks.logCb(3, e.what(), ctx->callbacks.logCbUserData);
    } catch (...) {
        if (ctx->callbacks.logCb && 3 /*ERROR*/ >= ctx->callbacks.logCbMinLevel)
            ctx->callbacks.logCb(3, "dasher_destroy: training flush failed: unknown exception", ctx->callbacks.logCbUserData);
    }
    delete ctx->intf;
    delete ctx;
}

DASHER_API void dasher_set_low_memory_mode(dasher_ctx* ctx, int enabled) {
    if (!ctx || !ctx->intf) return;
    ctx->lowMemory = (enabled != 0);
    ctx->intf->SetLowMemoryMode(ctx->lowMemory);
}

DASHER_API void dasher_set_screen_size(dasher_ctx* ctx, int width, int height) {
    if (!ctx || !ctx->intf || width <= 0 || height <= 0) return;

    if (!ctx->screen) {
        ctx->screen = std::make_unique<CommandScreen>(width, height);
        // Forward any text measurement callback registered before the screen
        // existed (frontends commonly wire callbacks before starting the engine).
        if (ctx->callbacks.textSizeCb) ctx->screen->SetTextSizeCallback(ctx->callbacks.textSizeCb, ctx->callbacks.textSizeCbUserData);
        ctx->intf->ChangeScreen(ctx->screen.get());
    } else {
        ctx->screen->SetSize(width, height);
        ctx->intf->ScreenResized(ctx->screen.get());
    }

    if (!ctx->realized) {
        // A previous failed Realize left the interface incrementally mutated
        // (CreateModules registers onto the existing module manager; other
        // components are rebuilt in place), so retrying Realize() on it could
        // retain or duplicate state from the failed attempt. Recreate the
        // interface first: the settings store and all ctx-level state (screen,
        // callbacks, pending alphabet) survive; the fresh Realize rebuilds
        // every component. PointerInput is owned by the old interface's module
        // manager, so null it before the delete — CreateModules re-establishes.
        if (ctx->engineError) {
            ctx->input = nullptr;
            delete ctx->intf;
            ctx->intf = new dasher_ctx::Interface(ctx->settings.get(), ctx);
            // The host's low-memory request was applied to the old interface;
            // reapply so the retry honours the memory constraint (review P1).
            ctx->intf->SetLowMemoryMode(ctx->lowMemory);
            ctx->intf->ChangeScreen(ctx->screen.get());
        }
        // On a fresh install (no settings file existed at create time),
        // apply the canonical default BEFORE Realize(). We use
        // SetStringParameter with the parameter-table default — NOT
        // ResetParameter, which only changes the in-memory value. A
        // pre-realize programmatic set (dasher_set_string_parameter)
        // persists to disk via SAVE_IMMEDIATELY; an in-memory-only reset
        // would leave that stale entry, and the supposedly discarded
        // filter would resurrect on the next restart. SetStringParameter
        // overwrites the disk entry, broadcasts the change, and the
        // value is correct when Realize()->CreateInputFilter() runs.
        if (!ctx->settingsFileExisted) {
            const auto defaultIt = Dasher::Settings::parameter_defaults.find(Dasher::SP_INPUT_FILTER);
            if (defaultIt != Dasher::Settings::parameter_defaults.end()) {
                const auto* defaultValue = std::get_if<std::string>(&defaultIt->second.value);
                if (defaultValue) {
                    ctx->settings->SetStringParameter(Dasher::SP_INPUT_FILTER, *defaultValue);
                }
            }
        }

        // A half-completed Realize leaves the interface in an
        // indeterminate state (e.g. a null node model). Setting realized
        // anyway made the next dasher_frame assert on that null model.
        // Latch the RFC 0009 error state instead: frame()/input no-op and
        // dasher_has_engine_error() reports it; the frontend can surface it.
        if (!capi::guarded_result(ctx, "dasher_set_screen_size: Realize failed", false,
                                  [&]() -> bool {
                                      capi::test_inject(ctx, DASHER_FAIL_INJECT_REALIZE);
                                      ctx->intf->Realize(nowMs());
                                      return true;
                                  })) {
            ctx->engineError = true;
            return;
        }
        ctx->realized = true;
        // A successful (re)realize rebuilt the interface from scratch, so an
        // engineError latched by a previous failed Realize is obsolete. That
        // failed-Realize path is the only one that latches while !realized
        // (mid-frame throws leave realized set and never re-enter this
        // block), so clearing here cannot mask a live fault. Without this,
        // one failed realize + a successful retry left the engine permanently
        // no-op'ing (review P1 on #77).
        ctx->engineError = false;

        if (!ctx->pendingAlphabet.empty()) {
            std::string pending = ctx->pendingAlphabet;
            ctx->pendingAlphabet.clear();
            ctx->intf->SetStringParameter(Dasher::SP_ALPHABET_ID, pending);
        }
    }

    if (ctx->input) ctx->input->SetBounds(width, height);
}

DASHER_API void dasher_mouse_move(dasher_ctx* ctx, float x, float y) {
    if (!ctx || !ctx->input) return;
    if (ctx->engineError) return;
    capi::guarded(ctx, "dasher_mouse_move", /*latch=*/true, [&] {
        capi::test_inject(ctx, DASHER_FAIL_INJECT_MOUSE_MOVE);
        ctx->input->SetPosition(x, y);
    });
}

DASHER_API void dasher_mouse_down(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    if (ctx->engineError) return;
    if (ctx->mouseDown) return;
    ctx->mouseDown = true;
    capi::guarded(ctx, "dasher_mouse_down", /*latch=*/true, [&] {
        capi::test_inject(ctx, DASHER_FAIL_INJECT_MOUSE_DOWN);
        // In circle start mode, clicking should NOT start/stop Dasher —
        // only hovering inside the circle should. (Steve Saling feedback)
        if (ctx->intf->GetLongParameter(Dasher::LP_START_MODE) == Dasher::Options::StartMode::circle_start) return;
        ctx->intf->SetBoolParameter(Dasher::BP_START_MOUSE, true);
        ctx->intf->KeyDown(inputTime(ctx), Dasher::Keys::Primary_Input);
    });
}

DASHER_API void dasher_mouse_up(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    if (ctx->engineError) return;
    if (!ctx->mouseDown) return;
    ctx->mouseDown = false;
    capi::guarded(ctx, "dasher_mouse_up", /*latch=*/true, [&] {
        capi::test_inject(ctx, DASHER_FAIL_INJECT_MOUSE_UP);
        ctx->intf->KeyUp(inputTime(ctx), Dasher::Keys::Primary_Input);
    });
}

DASHER_API void dasher_key_event(dasher_ctx* ctx, int key, int pressed) {
    if (!ctx || !ctx->intf) return;
    if (ctx->engineError) return;
    capi::guarded(ctx, "dasher_key_event", /*latch=*/true, [&] {
        capi::test_inject(ctx, DASHER_FAIL_INJECT_KEY_EVENT);
        auto vk = static_cast<Dasher::Keys::VirtualKey>(key);
        if (pressed) {
            ctx->intf->KeyDown(inputTime(ctx), vk);
        } else {
            ctx->intf->KeyUp(inputTime(ctx), vk);
        }
    });
}

DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms, int** out_commands, int* out_command_count,
                             char*** out_strings, int* out_string_count) {
    if (out_commands) *out_commands = nullptr;
    if (out_command_count) *out_command_count = 0;
    if (out_strings) *out_strings = nullptr;
    if (out_string_count) *out_string_count = 0;

    // Anchor the engine timeline before anything else: input events between
    // frames are stamped from this (see dasher_ctx::lastFrameMs).
    if (ctx) ctx->lastFrameMs = time_ms > 0 ? time_ms : 0;

    if (!ctx || !ctx->intf || !ctx->screen || !ctx->realized) return;
    if (ctx->engineError) return;

    capi::guarded(ctx, "dasher_frame", /*latch=*/true, [&] {
        capi::test_inject(ctx, DASHER_FAIL_INJECT_FRAME);
        ctx->screen->BeginFrame();
        ctx->intf->NewFrame(static_cast<unsigned long>((time_ms > 0) ? time_ms : 0), true);
        ctx->screen->BuildStringPtrs();

        if (out_commands) *out_commands = const_cast<int*>(reinterpret_cast<const int*>(ctx->screen->GetCommands()));
        if (out_command_count) *out_command_count = ctx->screen->GetCommandCount();
        if (out_strings) *out_strings = const_cast<char**>(ctx->screen->GetStringPtrs());
        if (out_string_count) *out_string_count = ctx->screen->GetStringCount();
    });
}

DASHER_API int dasher_has_engine_error(dasher_ctx* ctx) {
    return (ctx && ctx->engineError) ? 1 : 0;
}

DASHER_API const char* dasher_get_output_text(dasher_ctx* ctx) {
    if (!ctx) return "";
    ctx->scratch.tlString = ctx->editBuffer;
    return ctx->scratch.tlString.c_str();
}

DASHER_API void dasher_reset_output_text(dasher_ctx* ctx) {
    if (!ctx) return;
    ctx->editBuffer.clear();
    ctx->cursorPos = 0;
    ctx->rateTimestamps.clear();
    notify_buffer_cleared(ctx);
}

DASHER_API void dasher_reset(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    ctx->editBuffer.clear();
    ctx->cursorPos = 0;
    ctx->rateTimestamps.clear();
    ctx->intf->SetOffset(0, true);
    notify_buffer_cleared(ctx);
}

DASHER_API const char* dasher_get_alphabet_id(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ctx->scratch.tlString = ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID);
    return ctx->scratch.tlString.c_str();
}

DASHER_API void dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id) {
    if (!ctx || !ctx->intf || !alphabet_id) return;
    ctx->editBuffer.clear();
    ctx->cursorPos = 0;
    notify_buffer_cleared(ctx); // documented side effect: setting clears the buffer
    if (!ctx->realized) {
        ctx->pendingAlphabet = alphabet_id;
        return;
    }
    if (ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID) == alphabet_id) return;
    if (ctx->mouseDown) {
        ctx->intf->KeyUp(inputTime(ctx), Dasher::Keys::Primary_Input);
        ctx->mouseDown = false;
    }
    ctx->intf->SetStringParameter(Dasher::SP_ALPHABET_ID, alphabet_id);
}

DASHER_API int dasher_get_language_model_id(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return 0;
    return static_cast<int>(ctx->intf->GetLongParameter(Dasher::LP_LANGUAGE_MODEL_ID));
}

DASHER_API void dasher_set_language_model_id(dasher_ctx* ctx, int model_id) {
    if (!ctx || !ctx->intf) return;
    ctx->intf->SetLongParameter(Dasher::LP_LANGUAGE_MODEL_ID, static_cast<long>(model_id));
}

DASHER_API int dasher_get_speed_percent(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return 100;
    const double base = 160.0;
    return lround_int(ctx->intf->GetLongParameter(Dasher::LP_MAX_BITRATE) / base * 100.0);
}

DASHER_API void dasher_set_speed_percent(dasher_ctx* ctx, int percent) {
    if (!ctx || !ctx->intf) return;
    capi::guarded(ctx, "dasher_set_speed_percent", /*latch=*/false, [&] {
        const double base = 160.0;
        // Clamp to the engine's declared LP_MAX_BITRATE range rather than the
        // historic 20–400 %: that cap was raw 32–640, which silently truncated
        // the top of Dasher v5's speed range (v5 allowed raw 10–800, i.e. up
        // to 500 %). Frontend speed controls should take their bounds from the
        // same manifest (dasher_get_parameter_info).
        long min_bitrate = 1, max_bitrate = 1000;
        auto it = Dasher::Settings::parameter_defaults.find(Dasher::LP_MAX_BITRATE);
        if (it != Dasher::Settings::parameter_defaults.end() && it->second.max > 0) {
            min_bitrate = it->second.min;
            max_bitrate = it->second.max;
        }
        long bitrate = static_cast<long>(lround_int(percent / 100.0 * base));
        if (bitrate < min_bitrate) bitrate = min_bitrate;
        if (bitrate > max_bitrate) bitrate = max_bitrate;
        ctx->intf->SetLongParameter(Dasher::LP_MAX_BITRATE, bitrate);
    });
}

DASHER_API int dasher_get_bool_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return 0;
    char context[96];
    snprintf(context, sizeof(context), "dasher_get_bool_parameter key=%d", key);
    return capi::guarded_result(ctx, context, 0, [&]() -> int {
        return ctx->intf->GetBoolParameter(static_cast<Dasher::Parameter>(key)) ? 1 : 0;
    });
}

DASHER_API void dasher_set_bool_parameter(dasher_ctx* ctx, int key, int value) {
    if (!ctx || !ctx->intf) return;
    capi::guarded(ctx, "dasher_set_bool_parameter", /*latch=*/false, [&] {
        ctx->intf->SetBoolParameter(static_cast<Dasher::Parameter>(key), value != 0);
    });
}

DASHER_API long dasher_get_long_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return 0;
    char context[96];
    snprintf(context, sizeof(context), "dasher_get_long_parameter key=%d", key);
    return capi::guarded_result(ctx, context, 0L, [&]() -> long {
        return ctx->intf->GetLongParameter(static_cast<Dasher::Parameter>(key));
    });
}

DASHER_API void dasher_set_long_parameter(dasher_ctx* ctx, int key, long value) {
    if (!ctx || !ctx->intf) return;
    capi::guarded(ctx, "dasher_set_long_parameter", /*latch=*/false, [&] {
        ctx->intf->SetLongParameter(static_cast<Dasher::Parameter>(key), value);
    });
}

DASHER_API const char* dasher_get_string_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return "";
    char context[96];
    snprintf(context, sizeof(context), "dasher_get_string_parameter key=%d", key);
    // Error path note: returns the "" literal and leaves tlString stale
    // (the old code cleared it first). Unobservable — every tlString-backed
    // getter overwrites the buffer before returning it.
    return capi::guarded_result(ctx, context, "", [&]() -> const char* {
        ctx->scratch.tlString = ctx->intf->GetStringParameter(static_cast<Dasher::Parameter>(key));
        return ctx->scratch.tlString.c_str();
    });
}

DASHER_API void dasher_set_string_parameter(dasher_ctx* ctx, int key, const char* value) {
    if (!ctx || !ctx->intf || !value) return;
    capi::guarded(ctx, "dasher_set_string_parameter", /*latch=*/false, [&] {
        ctx->intf->SetStringParameter(static_cast<Dasher::Parameter>(key), value);
    });
}

// Color utility functions
DASHER_API int dasher_color_argb(int alpha, int red, int green, int blue) {
    return ((alpha & 0xFF) << 24) | ((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF);
}

DASHER_API int dasher_color_rgb(int red, int green, int blue) {
    return dasher_color_argb(255, red, green, blue);
}

DASHER_API int dasher_color_get_alpha(int argb) {
    return (argb >> 24) & 0xFF;
}

DASHER_API int dasher_color_get_red(int argb) {
    return (argb >> 16) & 0xFF;
}

DASHER_API int dasher_color_get_green(int argb) {
    return (argb >> 8) & 0xFF;
}

DASHER_API int dasher_color_get_blue(int argb) {
    return argb & 0xFF;
}


// ── Colour palettes ───────────────────────────────────────────────────────

DASHER_API int dasher_get_palette_count(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return 0;
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_COLOUR_ID);
    return static_cast<int>(names.size());
}

DASHER_API const char* dasher_get_palette_name(dasher_ctx* ctx, int index) {
    if (!ctx || !ctx->intf) return "";
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_COLOUR_ID);
    if (index < 0 || index >= static_cast<int>(names.size())) return "";
    ctx->scratch.stringValues = std::move(names);
    return ctx->scratch.stringValues[index].c_str();
}

DASHER_API const char* dasher_get_current_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ctx->scratch.tlString = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
    return ctx->scratch.tlString.c_str();
}

DASHER_API int dasher_get_palette_preview_colors(dasher_ctx* ctx, int index, int* out_colors) {
    if (!ctx || !ctx->intf || !out_colors) return -1;
    auto colorIO = ctx->intf->GetColorIO();
    if (!colorIO) return -1;
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_COLOUR_ID);
    if (index < 0 || index >= static_cast<int>(names.size())) return -1;
    const auto* palette = colorIO->FindPalette(names[index]);
    if (!palette) return -1;
    const auto& preview = palette->GetUIPreviewColors();
    for (int i = 0; i < 4; i++) {
        out_colors[i] = colorToARGB(preview[i]);
    }
    return 0;
}

DASHER_API void dasher_set_palette(dasher_ctx* ctx, const char* palette_name) {
    if (!ctx || !ctx->intf || !palette_name) return;
    if (ctx->mouseDown) {
        ctx->intf->KeyUp(inputTime(ctx), Dasher::Keys::Primary_Input);
        ctx->mouseDown = false;
    }
    // Route through the appearance model (RFC 0007): this sets the user's
    // preference for the current effective appearance (and defaults the other
    // side to the companion), then resolves. This keeps palette selection
    // consistent with light/dark mode and prevents the persistence leak that
    // direct SP_COLOUR_ID writes would cause. If the model hasn't been touched
    // yet, ensureAppearanceInitialised seeds it first.
    dasher_set_user_palette(ctx, palette_name);
}

// ── Alphabets ──────────────────────────────────────────────────────────────

DASHER_API int dasher_get_alphabet_count(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return 0;
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_ALPHABET_ID);
    return static_cast<int>(names.size());
}

DASHER_API const char* dasher_get_alphabet_name(dasher_ctx* ctx, int index) {
    if (!ctx || !ctx->intf) return "";
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_ALPHABET_ID);
    if (index < 0 || index >= static_cast<int>(names.size())) return "";
    ctx->scratch.stringValues = std::move(names);
    return ctx->scratch.stringValues[index].c_str();
}

// ── Game Mode ───────────────────────────────────────────────────────────────

DASHER_API int dasher_enter_game_mode(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return -1;
    if (ctx->intf->GetGameModule()) return 0;
    ctx->intf->EnterGameMode();
    return ctx->intf->GetGameModule() ? 0 : -1;
}

DASHER_API void dasher_leave_game_mode(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    if (ctx->intf->GetGameModule()) ctx->intf->LeaveGameMode();
}

DASHER_API int dasher_game_mode_active(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return 0;
    return ctx->intf->GetGameModule() ? 1 : 0;
}

DASHER_API void dasher_game_set_canvas_text(dasher_ctx* ctx, int enabled) {
    if (!ctx || !ctx->intf) return;
    auto* gm = ctx->intf->GetGameModule();
    if (gm) gm->SetCanvasTextEnabled(enabled != 0);
}

static std::string symbolsToText(const Dasher::CAlphInfo* alph, const std::vector<Dasher::symbol>& syms, int count) {
    std::string result;
    for (int i = 0; i < count && i < (int)syms.size(); i++) {
        result += alph->GetText(syms[i]);
    }
    return result;
}

DASHER_API const char* dasher_game_get_target_text(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    auto* gm = ctx->intf->GetGameModule();
    if (!gm) return "";
    const auto& syms = gm->GetTargetSymbols();
    ctx->scratch.gameTextBuf = symbolsToText(gm->GetAlphabet(), syms, (int)syms.size());
    return ctx->scratch.gameTextBuf.c_str();
}

DASHER_API int dasher_game_get_correct_count(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return -1;
    auto* gm = ctx->intf->GetGameModule();
    if (!gm) return -1;
    return gm->GetLastCorrectSym() + 1;
}

DASHER_API int dasher_game_get_target_length(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return -1;
    auto* gm = ctx->intf->GetGameModule();
    if (!gm) return -1;
    return (int)gm->GetTargetSymbols().size();
}

DASHER_API const char* dasher_game_get_wrong_text(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    auto* gm = ctx->intf->GetGameModule();
    if (!gm) return "";
    ctx->scratch.gameTextBuf = gm->GetWrongText();
    return ctx->scratch.gameTextBuf.c_str();
}

// ── Persistence ───────────────────────────────────────────────────────────

DASHER_API void dasher_save_settings(dasher_ctx* ctx) {
    if (!ctx || !ctx->settings) return;
    ctx->settings->Save();
    if (ctx->appearance.loaded) capi::saveAppearanceSettings(ctx); // RFC 0007 sidecar
}

DASHER_API void dasher_reload_settings(dasher_ctx* ctx) {
    if (!ctx || !ctx->settings) return;
    capi::guarded(ctx, "dasher_reload_settings", /*latch=*/false, [&] {
        // Re-read dasher_settings.xml and apply changes through the normal
        // parameter path — fires OnParameterChanged so the engine rebuilds
        // derived state (alphabet, colours, input filter) and the frontend
        // callback notifies. Safe to call any time; only differing values
        // are applied. Use cases: settings file changed externally (IME
        // service shared directory, migration, another process).
        ctx->settings->ReloadFromFile();
    });
}

// Reset every parameter to its built-in default value (from Parameters.h).
// Routes through the typed Set*Parameter methods so the normal parameter-change
// notifications fire and a live engine reconfigures itself (alphabet/colour/LM
// reload, etc.). Persistence files are left untouched; frontends that want
// persisted defaults delete dasher_settings.xml / appearance_settings.xml
// themselves before calling.
DASHER_API void dasher_reset_settings(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    capi::guarded(ctx, "dasher_reset_settings", /*latch=*/false, [&] {
        for (const auto& [param, entry] : Dasher::Settings::parameter_defaults) {
            const auto& value = entry.value;
            if (std::holds_alternative<bool>(value)) {
                ctx->intf->SetBoolParameter(param, std::get<bool>(value));
            } else if (std::holds_alternative<long>(value)) {
                ctx->intf->SetLongParameter(param, std::get<long>(value));
            } else if (std::holds_alternative<std::string>(value)) {
                ctx->intf->SetStringParameter(param, std::get<std::string>(value));
            }
        }
    });
}


DASHER_API void dasher_set_output_callback(dasher_ctx* ctx, dasher_output_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->callbacks.outputCb = callback;
    ctx->callbacks.outputCbUserData = user_data;
}

DASHER_API void dasher_set_text_size_callback(dasher_ctx* ctx, dasher_text_size_callback callback, void* user_data) {
    if (!ctx) return;
    // Keep on the ctx too: if the screen doesn't exist yet, creation forwards it.
    ctx->callbacks.textSizeCb = callback;
    ctx->callbacks.textSizeCbUserData = user_data;
    if (ctx->screen) ctx->screen->SetTextSizeCallback(callback, user_data);
}

DASHER_API void dasher_text_metrics_changed(dasher_ctx* ctx) {
    if (!ctx || !ctx->screen) return;
    ctx->screen->TextMetricsChanged();
}

DASHER_API void dasher_set_message_callback(dasher_ctx* ctx, dasher_message_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->callbacks.messageCb = callback;
    ctx->callbacks.messageCbUserData = user_data;
}

DASHER_API void dasher_set_log_callback(dasher_ctx* ctx, dasher_log_callback callback, void* user_data, int min_level) {
    if (!ctx) return;
    ctx->callbacks.logCb = callback;
    ctx->callbacks.logCbUserData = user_data;
    ctx->callbacks.logCbMinLevel = min_level;
}

DASHER_API void dasher_set_speak_callback(dasher_ctx* ctx, dasher_speak_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->callbacks.speakCb = callback;
    ctx->callbacks.speakCbUserData = user_data;
}

DASHER_API void dasher_set_clipboard_callback(dasher_ctx* ctx, dasher_clipboard_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->callbacks.clipboardCb = callback;
    ctx->callbacks.clipboardCbUserData = user_data;
}

DASHER_API void dasher_set_parameter_callback(dasher_ctx* ctx, dasher_parameter_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->callbacks.paramCb = callback;
    ctx->callbacks.paramCbUserData = user_data;
}

// ── Test / diagnostic hooks ────────────────────────────────────────────────

DASHER_API int dasher_get_probabilities(dasher_ctx* ctx, int* out_lbnds, int* out_hbnds, int max_out) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* model = ctx->intf->GetModel();
    if (!model) return -1;
    auto* node = model->Get_node_under_crosshair();
    if (!node) return -1;
    const auto& children = node->GetChildren();
    int count = 0;
    for (auto* child : children) {
        if (count >= max_out) break;
        if (out_lbnds) out_lbnds[count] = (int)child->Lbnd();
        if (out_hbnds) out_hbnds[count] = (int)child->Hbnd();
        count++;
    }
    return count;
}

DASHER_API int dasher_screen_to_dasher(dasher_ctx* ctx, int sx, int sy, long long* out_dx, long long* out_dy) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* view = ctx->intf->GetView();
    if (!view) return -1;
    Dasher::myint dx = 0, dy = 0;
    view->Screen2Dasher(static_cast<Dasher::screenint>(sx), static_cast<Dasher::screenint>(sy), dx, dy);
    if (out_dx) *out_dx = static_cast<long long>(dx);
    if (out_dy) *out_dy = static_cast<long long>(dy);
    return 0;
}

DASHER_API int dasher_dasher_to_screen(dasher_ctx* ctx, long long dx, long long dy, int* out_sx, int* out_sy) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* view = ctx->intf->GetView();
    if (!view) return -1;
    Dasher::screenint sx = 0, sy = 0;
    view->Dasher2Screen(static_cast<Dasher::myint>(dx), static_cast<Dasher::myint>(dy), sx, sy);
    if (out_sx) *out_sx = static_cast<int>(sx);
    if (out_sy) *out_sy = static_cast<int>(sy);
    return 0;
}

DASHER_API int dasher_get_root_child_count(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* model = ctx->intf->GetModel();
    if (!model) return -1;
    auto* node = model->Get_node_under_crosshair();
    if (!node) return -1;
    return static_cast<int>(node->ChildCount());
}

DASHER_API int dasher_get_root_child_bounds(dasher_ctx* ctx, int index, long long* out_lbnd, long long* out_hbnd) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* model = ctx->intf->GetModel();
    if (!model) return -1;
    auto* node = model->Get_node_under_crosshair();
    if (!node) return -1;
    const auto& children = node->GetChildren();
    if (index < 0 || index >= static_cast<int>(children.size())) return -1;
    auto* child = children[index];
    if (out_lbnd) *out_lbnd = static_cast<long long>(child->Lbnd());
    if (out_hbnd) *out_hbnd = static_cast<long long>(child->Hbnd());
    return 0;
}

DASHER_API int dasher_get_alphabet_symbol_count(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return -1;
    auto* alph = ctx->intf->GetActiveAlphabet();
    if (!alph) return -1;
    return alph->iEnd;
}

DASHER_API int dasher_get_alphabet_symbol_text(dasher_ctx* ctx, int index, char* out_text, int max_len) {
    if (!ctx || !ctx->intf || !out_text || max_len <= 0) return -1;
    auto* alph = ctx->intf->GetActiveAlphabet();
    if (!alph) return -1;
    if (index < 0 || index >= alph->iEnd) return -1;
    std::string text = alph->GetText(index);
    if (text.empty()) return -1;
    int len = std::min((int)text.size(), max_len - 1);
    std::memcpy(out_text, text.c_str(), len);
    out_text[len] = '\0';
    return 0;
}

DASHER_API int dasher_get_alphabet_symbol_image(dasher_ctx* ctx, int index, char* out_path, int max_len) {
    if (!ctx || !ctx->intf || !out_path || max_len <= 0) return -1;
    auto* alph = ctx->intf->GetActiveAlphabet();
    if (!alph) return -1;
    if (index < 0 || index >= alph->iEnd) return -1;
    std::string path = alph->GetImage(index);
    // Empty path means "no image" — still return 0 with empty string so the
    // frontend can distinguish "valid symbol, no image" from "error".
    int len = std::min((int)path.size(), max_len - 1);
    std::memcpy(out_path, path.c_str(), len);
    out_path[len] = '\0';
    return 0;
}

DASHER_API int dasher_get_alphabet_symbol_display(dasher_ctx* ctx, int index, char* out_text, int max_len) {
    if (!ctx || !ctx->intf || !out_text || max_len <= 0) return -1;
    auto* alph = ctx->intf->GetActiveAlphabet();
    if (!alph) return -1;
    if (index < 0 || index >= alph->iEnd) return -1;
    std::string text = alph->GetDisplayText(index);
    int len = std::min((int)text.size(), max_len - 1);
    std::memcpy(out_text, text.c_str(), len);
    out_text[len] = '\0';
    return 0;
}

DASHER_API int dasher_import_training_text(dasher_ctx* ctx, const char* text) {
    if (!ctx || !ctx->intf || !text) return -1;
    try {
        // Write text to a temp file since ImportTrainingText expects a path
        std::string tmpfile = ctx->userDir + "/.dasher_training_tmp.txt";
        std::ofstream out(tmpfile);
        if (!out) return -1;
        out << text;
        out.close();
        ctx->intf->ImportTrainingText(tmpfile);
        // ParseFile is synchronous — remove the temp file instead of
        // littering the user dir (it would also show up in user-dir scans).
        std::error_code ec;
        std::filesystem::remove(tmpfile, ec);
        return 0;
    } catch (...) {
        return -1;
    }
}

DASHER_API const char* dasher_get_training_path(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    try {
        // Resolve against THIS context's user dir, not the process-global
        // FileUtils directory (owned by whichever context was created last —
        // two live contexts with different dirs must not see each other's
        // training files).
        const std::string file = ctx->intf->GetAlphabetTrainingFile();
        if (file.empty() || ctx->userDir.empty())
            ctx->scratch.tlString.clear();
        else
            ctx->scratch.tlString = (std::filesystem::path(ctx->userDir) / file).string();
    } catch (...) {
        ctx->scratch.tlString.clear();
    }
    return ctx->scratch.tlString.c_str();
}

DASHER_API int dasher_capi_version(void) {
    return DASHER_CAPI_VERSION;
}

// Test hook: arm/disarm deterministic failure injection (todo.md 0.6).
// One integer store; the throw itself lives in capi::test_inject at each
// guarded entry point.
DASHER_API void dasher_test_inject_failure(dasher_ctx* ctx, int site) {
    if (!ctx) return;
    ctx->failInjectSite = site;
}


// ── Custom rendering, Strand 2 (RFC 0013) ──────────────────────────────────

DASHER_API int dasher_set_visible_nodes_enabled(dasher_ctx* ctx, int enabled) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    try {
        auto* view = ctx->intf->GetView();
        if (!view) return -1;
        view->SetVisibleNodeCapture(enabled != 0);
        return 0;
    } catch (...) {
        return -1;
    }
}

DASHER_API int dasher_get_visible_nodes(dasher_ctx* ctx, dasher_node_info* out_nodes, int max_nodes,
                                        char*** out_strings, int* out_string_count) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    if (!out_nodes || max_nodes <= 0) return -1;

    // CONTRIBUTING Rule 4: never let a C++ exception cross extern "C". Node
    // accessors (notably GetAlphSymbol, which throws on the base class) can
    // throw, so the whole body is guarded; the IsSymbolNode() check below keeps
    // the common path throw-free, and this catch is the safety net.
    try {
        auto* view = ctx->intf->GetView();
        if (!view || !view->IsVisibleNodeCaptureEnabled()) return -1;

        // ABI version check: the caller sets struct_size on the first element.
        const int caller_size = out_nodes[0].struct_size;
        const int v1_size = static_cast<int>(sizeof(dasher_node_info));
        if (caller_size < v1_size) {
            // Caller is built against an older, smaller struct than this engine.
            // Filling v1 fields would overflow their allocation.
            return -1;
        }

        const auto nodes = view->GetVisibleNodes(); // by value; stable copy

        ctx->scratch.nodeLabelStrings.clear();
        ctx->scratch.nodeLabelPtrs.clear();

        const int total = static_cast<int>(nodes.size());
        const int written = std::min(total, max_nodes);
        for (int i = 0; i < written; ++i) {
            const auto& n = nodes[i];
            dasher_node_info& out = out_nodes[i];
            out.struct_size = v1_size;
            out.dasher_y1 = static_cast<long long>(n.dasher_y1);
            out.dasher_y2 = static_cast<long long>(n.dasher_y2);
            // All node-derived values were resolved during Render() and stored
            // in the snapshot — no CDasherNode* is dereferenced here, so the
            // model is free to mutate/free nodes between the frame and this
            // query. (Previously this lazily dereferenced a stored pointer,
            // which dangled and crashed — review feedback on #51.)
            out.symbol = n.symbol;
            out.has_children = n.has_children;
            out.depth = n.depth;
            out.is_game_node = n.is_game_node;
            out.screen_x1 = n.screen_x1;
            out.screen_y1 = n.screen_y1;
            out.screen_x2 = n.screen_x2;
            out.screen_y2 = n.screen_y2;
            out.fill_argb = colorToARGB(n.fill);
            out.outline_argb = colorToARGB(n.outline);
            if (!n.label.empty()) {
                ctx->scratch.nodeLabelStrings.push_back(n.label);
                out.label_index = static_cast<int>(ctx->scratch.nodeLabelStrings.size() - 1);
            } else {
                out.label_index = -1;
            }
        }

        // Build char* pointers for the strings array.
        ctx->scratch.nodeLabelPtrs.resize(ctx->scratch.nodeLabelStrings.size());
        for (size_t i = 0; i < ctx->scratch.nodeLabelStrings.size(); ++i)
            ctx->scratch.nodeLabelPtrs[i] = ctx->scratch.nodeLabelStrings[i].data();

        if (out_strings) *out_strings = ctx->scratch.nodeLabelPtrs.data();
        if (out_string_count) *out_string_count = static_cast<int>(ctx->scratch.nodeLabelPtrs.size());

        // Return the total available count (may exceed max_nodes) so the caller
        // can grow its buffer and re-query if truncated.
        return total;
    } catch (...) {
        return -1;
    }
}

DASHER_API int dasher_get_viewport(dasher_ctx* ctx, dasher_viewport* out) {
    if (!ctx || !ctx->intf || !ctx->realized || !out) return -1;
    try {
        const int caller_size = out->struct_size;
        const int v1_size = static_cast<int>(sizeof(dasher_viewport));
        if (caller_size < v1_size) return -1;
        auto* view = ctx->intf->GetView();
        if (!view) return -1;
        const auto vr = view->VisibleRegion();
        out->struct_size = v1_size;
        out->crosshair_x = static_cast<long long>(Dasher::CDasherModel::ORIGIN_X);
        out->crosshair_y = static_cast<long long>(Dasher::CDasherModel::ORIGIN_Y);
        out->visible_min_y = static_cast<long long>(vr.minY);
        out->visible_max_y = static_cast<long long>(vr.maxY);
        out->screen_width = view->Screen()->GetWidth();
        out->screen_height = view->Screen()->GetHeight();
        return 0;
    } catch (...) {
        return -1;
    }
}

// ── Custom actions ─────────────────────────────────────────────────────────

DASHER_API void dasher_register_action(dasher_ctx* ctx, const char* name, dasher_action_callback callback,
                                       void* user_data) {
    if (!ctx || !name || !callback) return;
    ctx->customActions.push_back({std::string(name), callback, user_data});

    // If control manager already exists, register directly for immediate use
    if (ctx->intf) {
        auto* cm = ctx->intf->GetControlManager();
        if (cm) {
            auto cb = callback;
            auto ud = user_data;
            cm->GetActionRegistry()->registerCustomAction(
                std::string(name),
                [cb, ud](const std::string& actionName, const std::map<std::string, std::string>& attrs) {
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
                    cb(actionName.c_str(), static_cast<int>(keyPtrs.size()), keyPtrs.data(), valPtrs.data(), ud);
                });
        }
    }
}

// ── Typing rate (RFC 0012) ──────────────────────────────────────────────────

DASHER_API double dasher_get_cps(dasher_ctx* ctx) {
    if (!ctx) return 0.0;
    // Lazy-trim: remove timestamps older than 5 seconds.
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(5);
    while (!ctx->rateTimestamps.empty() && ctx->rateTimestamps.front() < cutoff) {
        ctx->rateTimestamps.pop_front();
    }
    if (ctx->rateTimestamps.empty()) return 0.0;
    // Window duration: from the oldest remaining timestamp to now.
    // Cap at 5s; floor at 1s for stability (avoids spikes on the first char).
    double elapsed = std::chrono::duration<double>(now - ctx->rateTimestamps.front()).count();
    if (elapsed < 1.0) elapsed = 1.0;
    double cps = static_cast<double>(ctx->rateTimestamps.size()) / elapsed;
    return cps;
}

DASHER_API double dasher_get_wpm(dasher_ctx* ctx) {
    return dasher_get_cps(ctx) * 12.0;
}

DASHER_API void dasher_reset_cps(dasher_ctx* ctx) {
    if (!ctx) return;
    ctx->rateTimestamps.clear();
}

} // extern "C"
