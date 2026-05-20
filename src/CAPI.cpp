#include "dasher.h"

#include "DasherCore/DashIntfScreenMsgs.h"
#include "DasherCore/DasherInput.h"
#include "DasherCore/DasherScreen.h"
#include "DasherCore/DasherInterfaceBase.h"
#include "DasherCore/ModuleManager.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/XmlSettingsStore.h"
#include "DasherCore/FileUtils.h"
#include "DasherCore/ColorPalette.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

static int lround_int(double v) {
    int i = static_cast<int>(v);
    return (v - i >= 0.5) ? i + 1 : (v - i <= -0.5) ? i - 1 : i;
}

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int32_t colorToARGB(const Dasher::ColorPalette::Color &c) {
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

// ── Command-buffer screen ──────────────────────────────────────────────────

class CommandScreen final : public Dasher::CDasherScreen {
public:
    CommandScreen(int width, int height)
        : CDasherScreen(static_cast<Dasher::screenint>(width), static_cast<Dasher::screenint>(height)) {}

    void SetSize(int width, int height) {
        resize(static_cast<Dasher::screenint>(width), static_cast<Dasher::screenint>(height));
    }

    void BeginFrame() {
        m_commands.clear();
        m_strings.clear();
        m_stringPtrs.clear();
        push(0, 0, 0, 0, 0, static_cast<int32_t>(0xFF0D1117));
    }

    void BuildStringPtrs() {
        m_stringPtrs.resize(m_strings.size());
        for (size_t i = 0; i < m_strings.size(); ++i)
            m_stringPtrs[i] = m_strings[i].data();
    }

    const int32_t *GetCommands() const { return m_commands.data(); }
    int GetCommandCount() const { return static_cast<int>(m_commands.size()); }
    char *const *GetStringPtrs() const { return m_stringPtrs.data(); }
    int GetStringCount() const { return static_cast<int>(m_strings.size()); }

    std::pair<Dasher::screenint, Dasher::screenint> TextSize(Label *label, unsigned int iFontSize) override {
        if (!label) return std::make_pair(Dasher::screenint(0), Dasher::screenint(0));
        return std::make_pair(
            static_cast<Dasher::screenint>(label->m_strText.size() * iFontSize / 2),
            static_cast<Dasher::screenint>(iFontSize));
    }

    void DrawString(Label *label, Dasher::screenint x, Dasher::screenint y, unsigned int iFontSize, const Dasher::ColorPalette::Color &color) override {
        if (!label || label->m_strText.empty() || iFontSize == 0) return;
        int idx = static_cast<int>(m_strings.size());
        m_strings.push_back(label->m_strText);
        push(5, x, y, static_cast<int>(iFontSize), idx, colorToARGB(color));
    }

    void DrawRectangle(Dasher::screenint x1, Dasher::screenint y1, Dasher::screenint x2, Dasher::screenint y2, const Dasher::ColorPalette::Color &color, const Dasher::ColorPalette::Color &outlineColor, int iThickness) override {
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
        if (iThickness > 0 && !outlineColor.isFullyTransparent()) push(3, x1, y1, x2, y2, colorToARGB(outlineColor));
    }

    void DrawCircle(Dasher::screenint cx, Dasher::screenint cy, Dasher::screenint r, const Dasher::ColorPalette::Color &fillColor, const Dasher::ColorPalette::Color &lineColor, int iLineWidth) override {
        if (!fillColor.isFullyTransparent()) push(1, cx, cy, r, 1, colorToARGB(fillColor));
        if (iLineWidth > 0 && !lineColor.isFullyTransparent()) push(1, cx, cy, r, 0, colorToARGB(lineColor));
    }

    void Polyline(Dasher::point *Points, int Number, int iWidth, const Dasher::ColorPalette::Color &color) override {
        if (!Points || Number < 2 || color.isFullyTransparent()) return;
        for (int i = 1; i < Number; ++i)
            push(2, Points[i-1].x, Points[i-1].y, Points[i].x, Points[i].y, colorToARGB(color));
    }

    void Polygon(Dasher::point *Points, int Number, const Dasher::ColorPalette::Color &fillColor, const Dasher::ColorPalette::Color &outlineColor, int lineWidth) override {
        if (!Points || Number < 2) return;
        if (!fillColor.isFullyTransparent()) {
            for (int i = 1; i < Number; ++i)
                push(2, Points[i-1].x, Points[i-1].y, Points[i].x, Points[i].y, colorToARGB(fillColor));
            push(2, Points[Number-1].x, Points[Number-1].y, Points[0].x, Points[0].y, colorToARGB(fillColor));
        }
        if (lineWidth > 0 && !outlineColor.isFullyTransparent()) {
            for (int i = 1; i < Number; ++i)
                push(2, Points[i-1].x, Points[i-1].y, Points[i].x, Points[i].y, colorToARGB(outlineColor));
            push(2, Points[Number-1].x, Points[Number-1].y, Points[0].x, Points[0].y, colorToARGB(outlineColor));
        }
    }

    void Display() override {}
    bool IsPointVisible(Dasher::screenint, Dasher::screenint) override { return true; }

private:
    void push(int op, int a, int b, int c, int d, int32_t colour) {
        m_commands.push_back(op);
        m_commands.push_back(a);
        m_commands.push_back(b);
        m_commands.push_back(c);
        m_commands.push_back(d);
        m_commands.push_back(colour);
    }

    std::vector<int32_t> m_commands;
    std::vector<std::string> m_strings;
    std::vector<char *> m_stringPtrs;
};

// ── Pointer input ─────────────────────────────────────────────────────────

class PointerInput : public Dasher::CScreenCoordInput {
public:
    PointerInput() : CScreenCoordInput("Pointer Input") {}

    void SetBounds(int w, int h) {
        m_width = (w > 0) ? w : 1;
        m_height = (h > 0) ? h : 1;
        if (!m_hasPos) { m_x = m_width / 2; m_y = m_height / 2; }
    }

    void SetPosition(float x, float y) {
        m_hasPos = true;
        m_x = clamp_int(lround_int(static_cast<double>(x)), 0, m_width - 1);
        m_y = clamp_int(lround_int(static_cast<double>(y)), 0, m_height - 1);
    }

    bool GetScreenCoords(Dasher::screenint &iX, Dasher::screenint &iY, Dasher::CDasherView *) override {
        iX = static_cast<Dasher::screenint>(m_x);
        iY = static_cast<Dasher::screenint>(m_y);
        return true;
    }

private:
    int m_width = 1, m_height = 1, m_x = 0, m_y = 0;
    bool m_hasPos = false;
};

// ── Helpers ────────────────────────────────────────────────────────────────

static unsigned long nowMs() {
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ── Session context ───────────────────────────────────────────────────────

struct dasher_ctx {
    struct Interface;
    std::unique_ptr<Dasher::XmlSettingsStore> settings;
    std::unique_ptr<CommandScreen> screen;
    PointerInput *input = nullptr;
    Dasher::CDashIntfScreenMsgs *intf = nullptr;
    std::string editBuffer;
    std::string tlString;
    bool realized = false;
    bool mouseDown = false;
    std::string pendingAlphabet;
    std::string dataDir;
    std::string stringBuf;

    struct Interface : public Dasher::CDashIntfScreenMsgs {
        Interface(Dasher::CSettingsStore *s, dasher_ctx *owner)
            : CDashIntfScreenMsgs(s), m_owner(owner) {}

        void CreateModules() override {
            CDashIntfScreenMsgs::CreateModules();
            auto inp = std::make_unique<PointerInput>();
            m_owner->input = inp.get();
            GetModuleManager()->RegisterInputDeviceModule(std::move(inp), true);
        }

        unsigned int ctrlMove(bool, Dasher::EditDistance) override {
            return static_cast<unsigned int>(m_owner->editBuffer.size());
        }
        unsigned int ctrlDelete(bool, Dasher::EditDistance) override {
            return static_cast<unsigned int>(m_owner->editBuffer.size());
        }
        void editOutput(const std::string &strText, Dasher::CDasherNode *pCause) override {
            m_owner->editBuffer += strText;
            CDashIntfScreenMsgs::editOutput(strText, pCause);
        }
        void editDelete(const std::string &strText, Dasher::CDasherNode *pCause) override {
            if (!strText.empty() && m_owner->editBuffer.size() >= strText.size())
                m_owner->editBuffer.erase(m_owner->editBuffer.size() - strText.size());
            CDashIntfScreenMsgs::editDelete(strText, pCause);
        }
        std::string GetContext(unsigned int start, unsigned int len) override {
            if (start >= m_owner->editBuffer.size()) return {};
            return m_owner->editBuffer.substr(start, len);
        }
        std::string GetAllContext() override { return m_owner->editBuffer; }
        int GetAllContextLenght() override { return static_cast<int>(m_owner->editBuffer.size()); }

        dasher_ctx *m_owner;
    };
};

// ── C API implementation ──────────────────────────────────────────────────

extern "C" {

static std::string s_errorString;
static std::string s_localeCode = "en";
static std::unordered_map<std::string, std::string> s_localeStrings;
static std::unordered_map<std::string, std::string> s_overrideStrings;

DASHER_API dasher_ctx* dasher_create(const char* data_dir, const char* user_dir,
                                      char** out_error) {
    if (out_error) *out_error = nullptr;
    if (!data_dir) {
        s_errorString = "data_dir is NULL";
        if (out_error) *out_error = s_errorString.data();
        return nullptr;
    }

    auto *ctx = new dasher_ctx();
    std::string dir(data_dir);
    ctx->dataDir = dir;
    std::string writableDir = user_dir ? std::string(user_dir) : dir;

    Dasher::FileUtils::SetDataDirectory(dir);

    std::string settingsPath = writableDir;
#ifdef _WIN32
    settingsPath += "\\dasher_settings.xml";
#else
    settingsPath += "/dasher_settings.xml";
#endif

    try {
        ctx->settings = std::make_unique<Dasher::XmlSettingsStore>(settingsPath, nullptr);
        ctx->settings->Load();
        ctx->intf = new dasher_ctx::Interface(ctx->settings.get(), ctx);
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

    return ctx;
}

DASHER_API void dasher_destroy(dasher_ctx* ctx) {
    if (!ctx) return;
    delete ctx->intf;
    delete ctx;
}

DASHER_API void dasher_set_screen_size(dasher_ctx* ctx, int width, int height) {
    if (!ctx || !ctx->intf || width <= 0 || height <= 0) return;

    if (!ctx->screen) {
        ctx->screen = std::make_unique<CommandScreen>(width, height);
        ctx->intf->ChangeScreen(ctx->screen.get());
    } else {
        ctx->screen->SetSize(width, height);
        ctx->intf->ScreenResized(ctx->screen.get());
    }

    if (!ctx->realized) {
        ctx->intf->Realize(nowMs());
        ctx->realized = true;
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
    ctx->input->SetPosition(x, y);
}

DASHER_API void dasher_mouse_down(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    if (ctx->mouseDown) return;
    ctx->mouseDown = true;
    ctx->intf->SetBoolParameter(Dasher::BP_START_MOUSE, true);
    ctx->intf->KeyDown(nowMs(), Dasher::Keys::Primary_Input);
}

DASHER_API void dasher_mouse_up(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    if (!ctx->mouseDown) return;
    ctx->mouseDown = false;
    ctx->intf->KeyUp(nowMs(), Dasher::Keys::Primary_Input);
}

DASHER_API void dasher_key_event(dasher_ctx* ctx, int key, int pressed) {
    if (!ctx || !ctx->intf) return;
    auto vk = static_cast<Dasher::Keys::VirtualKey>(key);
    if (pressed) {
        ctx->intf->KeyDown(nowMs(), vk);
    } else {
        ctx->intf->KeyUp(nowMs(), vk);
    }
}

DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms,
                              int** out_commands, int* out_command_count,
                              char*** out_strings, int* out_string_count) {
    if (out_commands) *out_commands = nullptr;
    if (out_command_count) *out_command_count = 0;
    if (out_strings) *out_strings = nullptr;
    if (out_string_count) *out_string_count = 0;

    if (!ctx || !ctx->intf || !ctx->screen || !ctx->realized) return;

    ctx->screen->BeginFrame();
    ctx->intf->NewFrame(static_cast<unsigned long>((time_ms > 0) ? time_ms : 0), true);
    ctx->screen->BuildStringPtrs();

    if (out_commands) *out_commands = const_cast<int*>(reinterpret_cast<const int*>(ctx->screen->GetCommands()));
    if (out_command_count) *out_command_count = ctx->screen->GetCommandCount();
    if (out_strings) *out_strings = const_cast<char**>(ctx->screen->GetStringPtrs());
    if (out_string_count) *out_string_count = ctx->screen->GetStringCount();
}

DASHER_API const char* dasher_get_output_text(dasher_ctx* ctx) {
    if (!ctx) return "";
    ctx->tlString = ctx->editBuffer;
    return ctx->tlString.c_str();
}

DASHER_API void dasher_reset_output_text(dasher_ctx* ctx) {
    if (!ctx) return;
    ctx->editBuffer.clear();
}

DASHER_API const char* dasher_get_alphabet_id(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ctx->tlString = ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID);
    return ctx->tlString.c_str();
}

DASHER_API void dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id) {
    if (!ctx || !ctx->intf || !alphabet_id) return;
    ctx->editBuffer.clear();
    if (!ctx->realized) { ctx->pendingAlphabet = alphabet_id; return; }
    if (ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID) == alphabet_id) return;
    if (ctx->mouseDown) { ctx->intf->KeyUp(nowMs(), Dasher::Keys::Primary_Input); ctx->mouseDown = false; }
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
    const double base = 160.0;
    const int clamped = (percent < 20) ? 20 : (percent > 400) ? 400 : percent;
    long bitrate = static_cast<long>(lround_int(clamped / 100.0 * base));
    if (bitrate < 1) bitrate = 1;
    ctx->intf->SetLongParameter(Dasher::LP_MAX_BITRATE, bitrate);
}

DASHER_API int dasher_get_bool_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return 0;
    try {
        return ctx->intf->GetBoolParameter(static_cast<Dasher::Parameter>(key)) ? 1 : 0;
    } catch (const std::bad_variant_access&) {
        fprintf(stderr, "DASHER: bad_variant_access in get_bool_parameter key=%d\n", key);
        return 0;
    }
}

DASHER_API void dasher_set_bool_parameter(dasher_ctx* ctx, int key, int value) {
    if (!ctx || !ctx->intf) return;
    ctx->intf->SetBoolParameter(static_cast<Dasher::Parameter>(key), value != 0);
}

DASHER_API long dasher_get_long_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return 0;
    try {
        return ctx->intf->GetLongParameter(static_cast<Dasher::Parameter>(key));
    } catch (const std::bad_variant_access&) {
        fprintf(stderr, "DASHER: bad_variant_access in get_long_parameter key=%d\n", key);
        return 0;
    }
}

DASHER_API void dasher_set_long_parameter(dasher_ctx* ctx, int key, long value) {
    if (!ctx || !ctx->intf) return;
    ctx->intf->SetLongParameter(static_cast<Dasher::Parameter>(key), value);
}

DASHER_API const char* dasher_get_string_parameter(dasher_ctx* ctx, int key) {
    if (!ctx || !ctx->intf) return "";
    try {
        ctx->tlString = ctx->intf->GetStringParameter(static_cast<Dasher::Parameter>(key));
    } catch (const std::bad_variant_access&) {
        fprintf(stderr, "DASHER: bad_variant_access in get_string_parameter key=%d\n", key);
        ctx->tlString = "";
    }
    return ctx->tlString.c_str();
}

DASHER_API void dasher_set_string_parameter(dasher_ctx* ctx, int key, const char* value) {
    if (!ctx || !ctx->intf || !value) return;
    ctx->intf->SetStringParameter(static_cast<Dasher::Parameter>(key), value);
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

// ── Static schema data ────────────────────────────────────────────────────

static std::vector<Dasher::Parameter> s_paramKeys;
static std::string s_paramInfoName;
static std::string s_paramInfoDesc;
static std::string s_paramInfoGroup;
static std::string s_paramInfoSubgroup;
static std::vector<std::string> s_enumStrings;
static std::vector<std::pair<std::string, int>> s_enumEntries;
static std::vector<std::string> s_stringValues;

static void ensureParamKeys() {
    if (!s_paramKeys.empty()) return;
    for (const auto& [key, val] : Dasher::Settings::parameter_defaults) {
        s_paramKeys.push_back(key);
    }
    std::sort(s_paramKeys.begin(), s_paramKeys.end());
}

static const char* parameterGroup(Dasher::Parameter key) {
    switch (key) {
        case Dasher::BP_DRAW_MOUSE_LINE: case Dasher::BP_DRAW_MOUSE:
        case Dasher::BP_CURVE_MOUSE_LINE: case Dasher::BP_START_MOUSE:
        case Dasher::BP_START_SPACE: case Dasher::BP_STOP_OUTSIDE:
        case Dasher::BP_AUTOCALIBRATE: case Dasher::BP_REMAP_XTREME:
        case Dasher::LP_START_MODE: case Dasher::LP_ORIENTATION:
        case Dasher::SP_INPUT_FILTER: case Dasher::SP_INPUT_DEVICE:
            return "Input";

        case Dasher::SP_ALPHABET_ID: case Dasher::LP_LANGUAGE_MODEL_ID:
        case Dasher::BP_LM_ADAPTIVE: case Dasher::LP_LM_ALPHA:
        case Dasher::LP_LM_BETA: case Dasher::LP_LM_MAX_ORDER:
        case Dasher::LP_LM_EXCLUSION: case Dasher::LP_LM_UPDATE_EXCLUSION:
        case Dasher::LP_LM_MIXTURE: case Dasher::LP_LM_WORD_ALPHA:
        case Dasher::LP_UNIFORM:
            return "Language";

        case Dasher::SP_COLOUR_ID: case Dasher::BP_PALETTE_CHANGE:
        case Dasher::LP_DASHER_FONTSIZE: case Dasher::LP_MESSAGE_FONTSIZE:
        case Dasher::LP_SHAPE_TYPE: case Dasher::LP_GEOMETRY:
        case Dasher::LP_LINE_WIDTH: case Dasher::LP_OUTLINE_WIDTH:
        case Dasher::LP_TEXT_PADDING: case Dasher::LP_NODE_BUDGET:
        case Dasher::LP_MIN_NODE_SIZE: case Dasher::LP_NONLINEAR_X:
        case Dasher::BP_NONLINEAR_Y: case Dasher::BP_SIMULATE_TRANSPARENCY:
        case Dasher::SP_DASHER_FONT:
            return "Appearance";

        case Dasher::LP_MAX_BITRATE: case Dasher::BP_AUTO_SPEEDCONTROL:
        case Dasher::BP_SLOW_START: case Dasher::LP_SLOW_START_TIME:
        case Dasher::LP_AUTOSPEED_SENSITIVITY: case Dasher::LP_DYNAMIC_SPEED_INC:
        case Dasher::LP_DYNAMIC_SPEED_FREQ: case Dasher::LP_DYNAMIC_SPEED_DEC:
        case Dasher::LP_TAP_TIME: case Dasher::LP_X_LIMIT_SPEED:
        case Dasher::LP_MARGIN_WIDTH: case Dasher::LP_TARGET_OFFSET:
            return "Speed";

        case Dasher::BP_SPEAK_ALL_ON_STOP: case Dasher::BP_SPEAK_WORDS:
        case Dasher::BP_COPY_ALL_ON_STOP: case Dasher::LP_MESSAGE_TIME:
            return "Output";

        case Dasher::BP_SMOOTH_PRESS_MODE: case Dasher::BP_SMOOTH_ONLY_FORWARD:
        case Dasher::BP_SMOOTH_DRAW_MOUSE: case Dasher::BP_SMOOTH_DRAW_MOUSE_LINE:
        case Dasher::LP_SMOOTH_TAU: case Dasher::BP_EXACT_DYNAMICS:
        case Dasher::BP_TURBO_MODE: case Dasher::BP_BACKOFF_BUTTON:
        case Dasher::BP_TWOBUTTON_REVERSE: case Dasher::BP_2B_INVERT_DOUBLE:
        case Dasher::LP_ZOOMSTEPS: case Dasher::LP_B: case Dasher::LP_S:
        case Dasher::LP_BUTTON_SCAN_TIME: case Dasher::LP_R: case Dasher::LP_RIGHTZOOM:
        case Dasher::LP_TWO_BUTTON_OFFSET: case Dasher::LP_HOLD_TIME:
        case Dasher::LP_MULTIPRESS_TIME: case Dasher::LP_TWO_PUSH_OUTER:
        case Dasher::LP_TWO_PUSH_LONG: case Dasher::LP_TWO_PUSH_SHORT:
        case Dasher::LP_TWO_PUSH_TOLERANCE: case Dasher::LP_DYNAMIC_BUTTON_LAG:
        case Dasher::LP_STATIC1B_TIME: case Dasher::LP_STATIC1B_ZOOM:
        case Dasher::LP_MAXZOOM: case Dasher::LP_CIRCLE_PERCENT:
        case Dasher::BP_TWO_PUSH_RELEASE_TIME: case Dasher::BP_GAME_HELP_DRAW_PATH:
        case Dasher::LP_GAME_HELP_DIST: case Dasher::LP_GAME_HELP_TIME:
        case Dasher::LP_MOUSEPOSDIST: case Dasher::LP_PY_PROB_SORT_THRES:
        case Dasher::SP_BUTTON_MAPPINGS: case Dasher::SP_JOYSTICK_XAXIS:
        case Dasher::SP_JOYSTICK_YAXIS: case Dasher::SP_GAME_TEXT_FILE:
        case Dasher::LP_SOCKET_PORT: case Dasher::LP_SOCKET_INPUT_X_MIN:
        case Dasher::LP_SOCKET_INPUT_X_MAX: case Dasher::LP_SOCKET_INPUT_Y_MIN:
        case Dasher::LP_SOCKET_INPUT_Y_MAX: case Dasher::LP_USER_LOG_LEVEL_MASK:
        case Dasher::LP_DEMO_SPRING: case Dasher::LP_DEMO_NOISE_MEM:
        case Dasher::LP_DEMO_NOISE_MAG: case Dasher::LP_FRAMERATE:
        case Dasher::SP_ALPHABET_1: case Dasher::SP_ALPHABET_2:
        case Dasher::SP_ALPHABET_3: case Dasher::SP_ALPHABET_4:
            return "Advanced";

        default:
            return "Other";
    }
}

DASHER_API int dasher_get_parameter_count(void) {
    return static_cast<int>(Dasher::Settings::parameter_defaults.size());
}

DASHER_API int dasher_get_parameter_info(int index, dasher_parameter_info* out) {
    if (!out) return -1;
    ensureParamKeys();
    if (index < 0 || index >= static_cast<int>(s_paramKeys.size())) return -1;

    auto key = s_paramKeys[index];
    auto it = Dasher::Settings::parameter_defaults.find(key);
    if (it == Dasher::Settings::parameter_defaults.end()) return -1;

    const auto& val = it->second;
    out->key = static_cast<int>(key);
    std::string keyStr = std::to_string(static_cast<int>(key));

    std::string nameKey = val.enumKeyName + ".label";
    auto nameIt = s_overrideStrings.find(nameKey);
    if (nameIt != s_overrideStrings.end()) {
        s_paramInfoName = nameIt->second;
    } else {
        nameIt = s_localeStrings.find(nameKey);
        if (nameIt != s_localeStrings.end()) {
            s_paramInfoName = nameIt->second;
        } else {
            s_paramInfoName = val.humanName.empty() ? val.storageName : val.humanName;
        }
    }
    out->name = s_paramInfoName.c_str();

    std::string descKey = val.enumKeyName + ".description";
    auto descIt = s_overrideStrings.find(descKey);
    if (descIt != s_overrideStrings.end()) {
        s_paramInfoDesc = descIt->second;
    } else {
        descIt = s_localeStrings.find(descKey);
        if (descIt != s_localeStrings.end()) {
            s_paramInfoDesc = descIt->second;
        } else {
            s_paramInfoDesc = val.humanDescription;
        }
    }
    out->desc = s_paramInfoDesc.c_str();
    out->type = static_cast<int>(val.type);
    out->ui_type = static_cast<int>(val.suggestedUI);
    out->min_val = val.min;
    out->max_val = val.max;
    out->step = val.step;
    out->advanced = val.advancedSetting ? 1 : 0;
    out->group = parameterGroup(key);
    s_paramInfoSubgroup = val.subgroup;
    out->subgroup = s_paramInfoSubgroup.c_str();
    return 0;
}

DASHER_API int dasher_get_parameter_enum_count(int key) {
    auto it = Dasher::Settings::parameter_defaults.find(static_cast<Dasher::Parameter>(key));
    if (it == Dasher::Settings::parameter_defaults.end()) return 0;
    return static_cast<int>(it->second.possibleValues.size());
}

DASHER_API const char* dasher_get_parameter_enum_name(int key, int index) {
    auto it = Dasher::Settings::parameter_defaults.find(static_cast<Dasher::Parameter>(key));
    if (it == Dasher::Settings::parameter_defaults.end()) return "";
    int i = 0;
    for (const auto& [name, value] : it->second.possibleValues) {
        if (i == index) {
            s_enumStrings.push_back(name);
            return s_enumStrings.back().c_str();
        }
        i++;
    }
    return "";
}

DASHER_API int dasher_get_parameter_enum_value(int key, int index) {
    auto it = Dasher::Settings::parameter_defaults.find(static_cast<Dasher::Parameter>(key));
    if (it == Dasher::Settings::parameter_defaults.end()) return 0;
    int i = 0;
    for (const auto& [name, value] : it->second.possibleValues) {
        if (i == index) return value;
        i++;
    }
    return 0;
}

DASHER_API int dasher_get_parameter_string_values(dasher_ctx* ctx, int key, const char** out_names, int max_out) {
    if (!out_names || max_out <= 0) return 0;
    s_stringValues.clear();

    if (ctx && ctx->intf) {
        s_stringValues = ctx->intf->GetPermittedValues(static_cast<Dasher::Parameter>(key));
    }

    int count = static_cast<int>(s_stringValues.size());
    if (count > max_out) count = max_out;
    for (int i = 0; i < count; i++) {
        out_names[i] = s_stringValues[i].c_str();
    }
    return count;
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
    s_stringValues = std::move(names);
    return s_stringValues[index].c_str();
}

DASHER_API const char* dasher_get_current_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ctx->tlString = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
    return ctx->tlString.c_str();
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
    if (ctx->mouseDown) { ctx->intf->KeyUp(nowMs(), Dasher::Keys::Primary_Input); ctx->mouseDown = false; }
    ctx->intf->SetStringParameter(Dasher::SP_COLOUR_ID, std::string(palette_name));
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
    s_stringValues = std::move(names);
    return s_stringValues[index].c_str();
}

// ── Persistence ───────────────────────────────────────────────────────────

DASHER_API void dasher_save_settings(dasher_ctx* ctx) {
    if (!ctx || !ctx->settings) return;
    ctx->settings->Save();
}

// ── Localization ──────────────────────────────────────────────────────────

static std::unordered_map<std::string, std::string> parseStringsJson(const std::string& content) {
    std::unordered_map<std::string, std::string> result;
    enum class State { Key, Colon, Value, Skip };
    State state = State::Skip;
    std::string key, value;
    bool inString = false;
    bool escape = false;
    bool buildingKey = true;
    int depth = 0;

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];

        if (escape) {
            if (buildingKey) key += c;
            else value += c;
            escape = false;
            continue;
        }

        if (c == '\\') {
            escape = true;
            continue;
        }

        if (c == '"') {
            if (inString) {
                inString = false;
                if (buildingKey && depth == 1) {
                    buildingKey = false;
                } else if (!buildingKey && depth == 1) {
                    result[key] = value;
                    key.clear();
                    value.clear();
                    buildingKey = true;
                }
            } else {
                inString = true;
            }
            continue;
        }

        if (inString) {
            if (buildingKey) key += c;
            else value += c;
            continue;
        }

        if (c == '{') depth++;
        else if (c == '}') depth--;
    }

    return result;
}

DASHER_API int dasher_set_locale(dasher_ctx* ctx, const char* locale) {
    if (!ctx) return -1;

    if (!locale || std::string(locale) == "en" || std::string(locale) == "") {
        s_localeCode = "en";
        s_localeStrings.clear();
        return 0;
    }

    std::string localeStr(locale);
    std::string path = ctx->dataDir;
#ifdef _WIN32
    path += "\\Strings\\strings_";
#else
    path += "/Strings/strings_";
#endif
    path += localeStr + ".json";

    std::ifstream file(path);
    if (!file.is_open()) return -1;

    std::stringstream ss;
    ss << file.rdbuf();
    s_localeStrings = parseStringsJson(ss.str());
    s_localeCode = localeStr;
    return 0;
}

DASHER_API const char* dasher_get_locale(dasher_ctx* ctx) {
    if (!ctx) return "en";
    ctx->stringBuf = s_localeCode;
    return ctx->stringBuf.c_str();
}

DASHER_API void dasher_set_string_override(dasher_ctx* ctx, const char* key, const char* value) {
    if (!ctx || !key) return;
    if (value) {
        s_overrideStrings[key] = value;
    } else {
        s_overrideStrings.erase(key);
    }
}

DASHER_API const char* dasher_get_localized_string(dasher_ctx* ctx, const char* key) {
    if (!ctx || !key) return nullptr;
    auto it = s_overrideStrings.find(key);
    if (it != s_overrideStrings.end()) {
        ctx->stringBuf = it->second;
        return ctx->stringBuf.c_str();
    }
    it = s_localeStrings.find(key);
    if (it != s_localeStrings.end()) {
        ctx->stringBuf = it->second;
        return ctx->stringBuf.c_str();
    }
    return nullptr;
}

} // extern "C"
