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
#include "DasherCore/ColorIO.h"
#include "DasherCore/GameModule.h"
#include "DasherCore/Alphabet/AlphInfo.h"
#include "DasherCore/Alphabet/AlphIO.h"
#include "DasherCore/DasherModel.h"
#include "DasherCore/DasherNode.h"
#include "DasherCore/NodeCreationManager.h"
#include "DasherCore/ControlManager.h"
#include "DasherCore/LanguageModelling/LMRegistry.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "pugixml.hpp"

// ── UTF-8 / text boundary helpers ──────────────────────────────────────────

namespace {
size_t utf8CharLen(const std::string& s, size_t pos) {
    if (pos >= s.size()) {
        return 0;
    }
    const auto c = static_cast<unsigned char>(s[pos]);
    if (c < 0x80) {
        return 1;
    }
    if (c < 0xC0) {
        return 1; // continuation byte
    }
    if (c < 0xE0) {
        return 2;
    }
    if (c < 0xF0) {
        return 3;
    }
    return 4;
}

size_t nextChar(const std::string& s, size_t pos) {
    const size_t step = utf8CharLen(s, pos);
    return std::min(pos + (step > 0 ? step : 1), s.size());
}

size_t prevChar(const std::string& s, size_t pos) {
    if (pos == 0) {
        return 0;
    }
    size_t p = pos - 1;
    while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80) {
        --p;
    }
    return p;
}

bool isSep(char c, const char* seps) {
    return std::strchr(seps, c) != nullptr;
}

// Forward search: skip non-separators, then skip separators
size_t findAfter(const std::string& s, size_t pos, const char* seps) {
    if (pos > s.size()) pos = s.size();
    size_t p = pos;
    while (p < s.size() && !isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    while (p < s.size() && isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    return p;
}

// Backward search: skip separators, then skip non-separators
size_t findBefore(const std::string& s, size_t pos, const char* seps) {
    if (pos >= s.size()) pos = s.size() > 0 ? s.size() - 1 : 0;
    size_t p = pos;
    while (p > 0 && isSep(s[p], seps)) {
        p = prevChar(s, p);
    }
    while (p > 0 && !isSep(s[p], seps)) {
        p = prevChar(s, p);
    }
    // if we landed on a separator, advance one
    if (p < s.size() && isSep(s[p], seps)) {
        p = nextChar(s, p);
    }
    return p;
}

constexpr const char* kWordSeps = " \t\v\f\r\n";
constexpr const char* kSentenceSeps = ".?!\r\n";
constexpr const char* kParagraphSeps = "\r\n";

void getRange(const std::string& buf, bool bForwards, Dasher::EditDistance dist, size_t& ioStart, size_t& ioEnd) {
    switch (dist) {
    case Dasher::EDIT_CHAR:
        if (bForwards) {
            ioEnd = std::min(nextChar(buf, ioEnd), buf.size());
        } else {
            ioStart = prevChar(buf, ioStart);
        }
        break;
    case Dasher::EDIT_WORD:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kWordSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kWordSeps);
        }
        break;
    case Dasher::EDIT_SENTENCE:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kSentenceSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kSentenceSeps);
        }
        break;
    case Dasher::EDIT_PARAGRAPH:
    case Dasher::EDIT_LINE:
        if (bForwards) {
            ioEnd = findAfter(buf, ioEnd, kParagraphSeps);
        } else {
            ioStart = findBefore(buf, ioEnd > 0 ? ioEnd - 1 : 0, kParagraphSeps);
        }
        break;
    case Dasher::EDIT_FILE:
    case Dasher::EDIT_ALL:
    case Dasher::EDIT_PAGE:
    case Dasher::EDIT_SELECTION:
    case Dasher::EDIT_NONE:
        if (bForwards) {
            ioEnd = buf.size();
        } else {
            ioStart = 0;
        }
        break;
    }
}
} // namespace
#include <fstream>
#include <locale.h>
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

static int32_t colorToARGB(const Dasher::ColorPalette::Color& c) {
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

    const int32_t* GetCommands() const { return m_commands.data(); }
    int GetCommandCount() const { return static_cast<int>(m_commands.size()); }
    char* const* GetStringPtrs() const { return m_stringPtrs.data(); }
    int GetStringCount() const { return static_cast<int>(m_strings.size()); }

    std::pair<Dasher::screenint, Dasher::screenint> TextSize(Label* label, unsigned int iFontSize) override {
        if (!label) return std::make_pair(Dasher::screenint(0), Dasher::screenint(0));
        return std::make_pair(static_cast<Dasher::screenint>(label->m_strText.size() * iFontSize / 2),
                              static_cast<Dasher::screenint>(iFontSize));
    }

    void DrawString(Label* label, Dasher::screenint x, Dasher::screenint y, unsigned int iFontSize,
                    const Dasher::ColorPalette::Color& color) override {
        if (!label || label->m_strText.empty() || iFontSize == 0) return;
        int idx = static_cast<int>(m_strings.size());
        m_strings.push_back(label->m_strText);
        push(5, x, y, static_cast<int>(iFontSize), idx, colorToARGB(color));
    }

    void DrawRectangle(Dasher::screenint x1, Dasher::screenint y1, Dasher::screenint x2, Dasher::screenint y2,
                       const Dasher::ColorPalette::Color& color, const Dasher::ColorPalette::Color& outlineColor,
                       int iThickness) override {
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
        if (iThickness > 0 && !outlineColor.isFullyTransparent()) push(3, x1, y1, x2, y2, colorToARGB(outlineColor));
    }

    void DrawCircle(Dasher::screenint cx, Dasher::screenint cy, Dasher::screenint r,
                    const Dasher::ColorPalette::Color& fillColor, const Dasher::ColorPalette::Color& lineColor,
                    int iLineWidth) override {
        if (!fillColor.isFullyTransparent()) push(1, cx, cy, r, 1, colorToARGB(fillColor));
        if (iLineWidth > 0 && !lineColor.isFullyTransparent()) push(1, cx, cy, r, 0, colorToARGB(lineColor));
    }

    void Polyline(Dasher::point* Points, int Number, int iWidth, const Dasher::ColorPalette::Color& color) override {
        if (!Points || Number < 2 || color.isFullyTransparent()) return;
        // Opcode 6: set line width for subsequent line segments
        push(6, iWidth, 0, 0, 0, 0);
        for (int i = 1; i < Number; ++i)
            push(2, Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y, colorToARGB(color));
    }

    void Polygon(Dasher::point* Points, int Number, const Dasher::ColorPalette::Color& fillColor,
                 const Dasher::ColorPalette::Color& outlineColor, int lineWidth) override {
        if (!Points || Number < 2) return;
        if (!fillColor.isFullyTransparent()) {
            for (int i = 1; i < Number; ++i)
                push(2, Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y, colorToARGB(fillColor));
            push(2, Points[Number - 1].x, Points[Number - 1].y, Points[0].x, Points[0].y, colorToARGB(fillColor));
        }
        if (lineWidth > 0 && !outlineColor.isFullyTransparent()) {
            for (int i = 1; i < Number; ++i)
                push(2, Points[i - 1].x, Points[i - 1].y, Points[i].x, Points[i].y, colorToARGB(outlineColor));
            push(2, Points[Number - 1].x, Points[Number - 1].y, Points[0].x, Points[0].y, colorToARGB(outlineColor));
        }
    }

    void Display() override {}
    bool IsPointVisible(Dasher::screenint x, Dasher::screenint y) override {
        return x >= 0 && y >= 0 && x < GetWidth() && y < GetHeight();
    }

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
    std::vector<char*> m_stringPtrs;
};

// ── Pointer input ─────────────────────────────────────────────────────────

class PointerInput : public Dasher::CScreenCoordInput {
  public:
    PointerInput() : CScreenCoordInput("Pointer Input") {}

    void SetBounds(int w, int h) {
        m_width = (w > 0) ? w : 1;
        m_height = (h > 0) ? h : 1;
        if (!m_hasPos) {
            m_x = m_width / 2;
            m_y = m_height / 2;
        }
    }

    void SetPosition(float x, float y) {
        m_hasPos = true;
        // Don't clamp — allow out-of-bounds coordinates so BP_STOP_OUTSIDE
        // can detect when the pointer leaves the canvas area.
        m_x = lround_int(static_cast<double>(x));
        m_y = lround_int(static_cast<double>(y));
    }

    bool GetScreenCoords(Dasher::screenint& iX, Dasher::screenint& iY, Dasher::CDasherView*) override {
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
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

// ── Session context ───────────────────────────────────────────────────────

struct dasher_ctx {
    struct Interface;
    std::unique_ptr<Dasher::XmlSettingsStore> settings;
    std::unique_ptr<CommandScreen> screen;
    PointerInput* input = nullptr;
    Dasher::CDashIntfScreenMsgs* intf = nullptr;
    std::string editBuffer;
    size_t cursorPos = 0;
    std::string tlString;
    bool realized = false;
    bool mouseDown = false;
    std::string pendingAlphabet;
    std::string dataDir;
    std::string userDir;
    std::string stringBuf;

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
    void* outputCbUserData = nullptr;
    dasher_message_callback messageCb = nullptr;
    void* messageCbUserData = nullptr;
    dasher_speak_callback speakCb = nullptr;
    void* speakCbUserData = nullptr;
    dasher_clipboard_callback clipboardCb = nullptr;
    void* clipboardCbUserData = nullptr;
    dasher_parameter_callback paramCb = nullptr;
    void* paramCbUserData = nullptr;

    struct CustomActionEntry {
        std::string name;
        dasher_action_callback callback;
        void* userData;
    };
    std::vector<CustomActionEntry> customActions;

    struct Interface : public Dasher::CDashIntfScreenMsgs {
        Interface(Dasher::CSettingsStore* s, dasher_ctx* owner) : CDashIntfScreenMsgs(s), m_owner(owner) {
            s->OnParameterChanged.Subscribe(m_owner, [this](Dasher::Parameter param) {
                if (m_owner->paramCb) m_owner->paramCb(static_cast<int>(param), m_owner->paramCbUserData);
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
            if (m_owner->messageCb && !strText.empty())
                m_owner->messageCb(bInterrupt ? 1 : 0, strText.c_str(), m_owner->messageCbUserData);
            if (!m_owner->messageCb) CDashIntfScreenMsgs::Message(strText, bInterrupt);
        }

        unsigned int ctrlOffsetAfterMove(unsigned int offsetBefore, bool bForwards,
                                         Dasher::EditDistance dist) override {
            // offsetBefore is the model's node offset, which can exceed editBuffer.size()
            // because control nodes have offsets but produce no edit buffer characters.
            // Clamp to buffer bounds before using as an index.
            size_t bufLen = m_owner->editBuffer.size();
            size_t clamped = std::min(static_cast<size_t>(offsetBefore), bufLen);
            size_t start = clamped, end = clamped;
            getRange(m_owner->editBuffer, bForwards, dist, start, end);
            return static_cast<unsigned int>(bForwards ? end : start);
        }

        unsigned int ctrlMove(bool bForwards, Dasher::EditDistance dist) override {
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            getRange(m_owner->editBuffer, bForwards, dist, start, end);
            m_owner->cursorPos = bForwards ? end : start;
            return static_cast<unsigned int>(m_owner->cursorPos);
        }

        unsigned int ctrlDelete(bool bForwards, Dasher::EditDistance dist) override {
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            getRange(m_owner->editBuffer, bForwards, dist, start, end);
            if (start == end) {
                return static_cast<unsigned int>(m_owner->cursorPos);
            }

            const auto len = static_cast<size_t>(std::abs(static_cast<long>(end) - static_cast<long>(start)));
            const auto pos = std::min(start, end);
            const std::string deleted = m_owner->editBuffer.substr(pos, len);
            m_owner->editBuffer.erase(pos, len);
            m_owner->cursorPos = pos;

            if (m_owner->outputCb && !deleted.empty()) {
                m_owner->outputCb(1, deleted.c_str(), m_owner->outputCbUserData);
            }

            return static_cast<unsigned int>(m_owner->cursorPos);
        }
        void editOutput(const std::string& strText, Dasher::CDasherNode* pCause) override {
            if (m_owner->cursorPos > m_owner->editBuffer.size()) {
                m_owner->cursorPos = m_owner->editBuffer.size();
            }
            m_owner->editBuffer.insert(m_owner->cursorPos, strText);
            m_owner->cursorPos += strText.size();
            if (m_owner->outputCb && !strText.empty()) m_owner->outputCb(0, strText.c_str(), m_owner->outputCbUserData);
            CDashIntfScreenMsgs::editOutput(strText, pCause);
        }
        void editDelete(const std::string& strText, Dasher::CDasherNode* pCause) override {
            if (!strText.empty() && m_owner->editBuffer.size() >= strText.size() &&
                m_owner->cursorPos >= strText.size()) {
                m_owner->cursorPos -= strText.size();
                m_owner->editBuffer.erase(m_owner->cursorPos, strText.size());
            }
            if (m_owner->outputCb && !strText.empty()) m_owner->outputCb(1, strText.c_str(), m_owner->outputCbUserData);
            CDashIntfScreenMsgs::editDelete(strText, pCause);
        }
        std::string GetContext(unsigned int start, unsigned int len) override {
            if (start >= m_owner->editBuffer.size()) return {};
            return m_owner->editBuffer.substr(start, len);
        }
        std::string GetAllContext() override { return m_owner->editBuffer; }
        int GetAllContextLenght() override { return static_cast<int>(m_owner->editBuffer.size()); }

        bool SupportsSpeech() override { return m_owner->speakCb != nullptr; }

        void Speak(const std::string& text, bool bInterrupt) override {
            if (m_owner->speakCb && !text.empty())
                m_owner->speakCb(text.c_str(), bInterrupt ? 1 : 0, m_owner->speakCbUserData);
        }

        bool SupportsClipboard() override { return m_owner->clipboardCb != nullptr; }

        void CopyToClipboard(const std::string& text) override {
            if (m_owner->clipboardCb && !text.empty()) {
                m_owner->clipboardCb(text.c_str(), m_owner->clipboardCbUserData);
            }
        }

        std::string GetTextAroundCursor(Dasher::EditDistance dist) override {
            const std::string& buf = m_owner->editBuffer;
            size_t start = m_owner->cursorPos, end = m_owner->cursorPos;
            // Find the extent of text around cursor: forward then backward
            getRange(buf, true, dist, start, end);
            start = m_owner->cursorPos;
            getRange(buf, false, dist, start, end);
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
};

// ── C API implementation ──────────────────────────────────────────────────

// Appearance model helpers (RFC 0007). Defined outside `extern "C"` because they
// use C++ types (std::string, pugixml). Called by the C-linkage API functions
// and by dasher_create below.
namespace {
// Bidirectional companion lookup. Returns the opposite-appearance partner
// palette, or nullptr if none. Explicit `companion` first; then a reverse scan
// so legacy palettes without metadata are still paired.
const Dasher::ColorPalette* companionLookup(Dasher::CColorIO* colorIO, const std::string& name) {
    if (!colorIO) return nullptr;
    const Dasher::ColorPalette* p = colorIO->FindPalette(name);
    if (!p || p->PaletteName != name) return nullptr; // FindPalette falls back to default

    if (!p->CompanionName.empty()) {
        const Dasher::ColorPalette* q = colorIO->FindPalette(p->CompanionName);
        if (q && q->PaletteName == p->CompanionName && q != p) return q;
    }
    const auto* all = colorIO->GetKnownPalettes();
    for (const auto& [n, q] : *all) {
        if (q == p) continue;
        if (q->CompanionName == name) return q;
    }
    return nullptr;
}

// Effective appearance (1=light, 2=dark) from mode + transient system input.
int effectiveAppearanceValue(const dasher_ctx* ctx) {
    if (ctx->appearanceMode == 1) return 1; // forced light
    if (ctx->appearanceMode == 2) return 2; // forced dark
    return ctx->systemAppearance;           // follow system (defaults to light)
}

// Recompute the active palette from mode + system + preferences and write it to
// SP_COLOUR_ID (what the canvas renders). The persisted preferences are the
// source of truth, so this can never clobber the user's explicit choice.
void resolveAppearance(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;

    // Late seed: ensureAppearanceInitialised may have run before Realize,
    // when ColorIO wasn't available and the companion lookup failed.
    // Retry now — once ColorIO exists, the lookup succeeds and fills the gap.
    if (ctx->darkPalette.empty() && !ctx->lightPalette.empty()) {
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, ctx->lightPalette))
                ctx->darkPalette = comp->PaletteName;
        }
    }

    int eff = effectiveAppearanceValue(ctx);
    std::string target = (eff == 1) ? ctx->lightPalette : ctx->darkPalette;
    if (target.empty()) target = (eff == 1) ? ctx->darkPalette : ctx->lightPalette; // other side
    if (target.empty()) return; // nothing chosen yet; leave the engine default

    std::string current = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
    if (current != target) ctx->intf->SetStringParameter(Dasher::SP_COLOUR_ID, target);
}

std::string appearanceSettingsPath(const dasher_ctx* ctx) {
    std::string p = ctx->userDir;
#ifdef _WIN32
    p += "\\appearance_settings.xml";
#else
    p += "/appearance_settings.xml";
#endif
    return p;
}

// Load mode + light/dark preferences from the sidecar. Non-fatal on any error.
void loadAppearanceSettings(dasher_ctx* ctx) {
    if (ctx->appearanceLoaded) return;
    ctx->appearanceLoaded = true;
    std::string path = appearanceSettingsPath(ctx);
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(path.c_str());
    if (!res) return; // missing/unreadable: leave defaults
    pugi::xml_node root = doc.child("appearance");
    if (!root) return;
    ctx->appearanceMode = root.attribute("mode").as_int(0);
    ctx->lightPalette = root.attribute("light").as_string("");
    ctx->darkPalette = root.attribute("dark").as_string("");
    if (ctx->appearanceMode < 0 || ctx->appearanceMode > 2) ctx->appearanceMode = 0;
}

// Persist mode + light/dark preferences to the sidecar. Non-fatal on any error.
void saveAppearanceSettings(dasher_ctx* ctx) {
    if (!ctx || ctx->userDir.empty()) return;
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("appearance");
    root.append_attribute("mode") = ctx->appearanceMode;
    root.append_attribute("light") = ctx->lightPalette.c_str();
    root.append_attribute("dark") = ctx->darkPalette.c_str();
    doc.save_file(appearanceSettingsPath(ctx).c_str());
}

// Ensure the model is initialised: on first use, seed the light preference from
// the engine's current palette and default the dark side to its companion.
void ensureAppearanceInitialised(dasher_ctx* ctx) {
    if (ctx->appearanceLoaded) {
        resolveAppearance(ctx);
        return;
    }
    loadAppearanceSettings(ctx);
    if (ctx->lightPalette.empty() && ctx->darkPalette.empty()) {
        // Fresh start: adopt whatever palette the engine loaded as the light
        // preference, and default the dark side to its companion.
        std::string current = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
        ctx->lightPalette = current;
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, current))
                ctx->darkPalette = comp->PaletteName;
        }
        saveAppearanceSettings(ctx);
    }
    resolveAppearance(ctx);
}
} // namespace

extern "C" {

static std::string s_errorString;
static std::string s_localeCode = "en";
static std::unordered_map<std::string, std::string> s_localeStrings;
static std::unordered_map<std::string, std::string> s_overrideStrings;

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

    // Load persisted appearance preferences and resolve the active palette, so
    // the user's explicit choice is reflected immediately on startup (RFC 0007).
    loadAppearanceSettings(ctx);
    resolveAppearance(ctx);

    return ctx;
}

DASHER_API void dasher_destroy(dasher_ctx* ctx) {
    if (!ctx) return;
    delete ctx->intf;
    delete ctx;
}

DASHER_API void dasher_set_low_memory_mode(dasher_ctx* ctx, int enabled) {
    if (!ctx || !ctx->intf) return;
    ctx->intf->SetLowMemoryMode(enabled != 0);
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
        try {
            ctx->intf->Realize(nowMs());
        } catch (...) { // NOLINT(bugprone-empty-catch)
        }
        ctx->realized = true;

        // Force Normal Control filter for continuous mouse input
        ctx->intf->SetStringParameter(Dasher::SP_INPUT_FILTER, "Normal Control");

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

    // In circle start mode, clicking should NOT start/stop Dasher —
    // only hovering inside the circle should. (Steve Saling feedback)
    if (ctx->intf->GetLongParameter(Dasher::LP_START_MODE) == Dasher::Options::StartMode::circle_start) return;

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

DASHER_API void dasher_frame(dasher_ctx* ctx, int64_t time_ms, int** out_commands, int* out_command_count,
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
    ctx->cursorPos = 0;
}

DASHER_API void dasher_reset(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;
    ctx->editBuffer.clear();
    ctx->cursorPos = 0;
    ctx->intf->SetOffset(0, true);
}

DASHER_API const char* dasher_get_alphabet_id(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ctx->tlString = ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID);
    return ctx->tlString.c_str();
}

DASHER_API void dasher_set_alphabet_id(dasher_ctx* ctx, const char* alphabet_id) {
    if (!ctx || !ctx->intf || !alphabet_id) return;
    ctx->editBuffer.clear();
    ctx->cursorPos = 0;
    if (!ctx->realized) {
        ctx->pendingAlphabet = alphabet_id;
        return;
    }
    if (ctx->intf->GetStringParameter(Dasher::SP_ALPHABET_ID) == alphabet_id) return;
    if (ctx->mouseDown) {
        ctx->intf->KeyUp(nowMs(), Dasher::Keys::Primary_Input);
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

DASHER_API int dasher_get_language_model_count(void) {
    return Dasher::LMRegistry::instance().count();
}

DASHER_API int dasher_get_language_model_id_at(int index) {
    const auto& all = Dasher::LMRegistry::instance().all();
    if (index < 0 || index >= static_cast<int>(all.size())) return -1;
    return all[index].id;
}

DASHER_API const char* dasher_get_language_model_name(int id) {
    static std::string s_buf;
    auto* desc = Dasher::LMRegistry::instance().get(id);
    if (!desc) return "Unknown";
    s_buf = desc->name;
    return s_buf.c_str();
}

DASHER_API const char* dasher_get_language_model_description(int id) {
    static std::string s_buf;
    auto* desc = Dasher::LMRegistry::instance().get(id);
    if (!desc) return "";
    s_buf = desc->description;
    return s_buf.c_str();
}

DASHER_API int dasher_get_language_model_param_count(int id) {
    auto* desc = Dasher::LMRegistry::instance().get(id);
    if (!desc) return 0;
    return static_cast<int>(desc->paramKeys.size());
}

DASHER_API int dasher_get_language_model_param_key(int id, int index) {
    auto* desc = Dasher::LMRegistry::instance().get(id);
    if (!desc || index < 0 || index >= static_cast<int>(desc->paramKeys.size())) return -1;
    return desc->paramKeys[index];
}

DASHER_API int dasher_find_parameter_key(const char* enum_key_name) {
    if (!enum_key_name) return -1;
    std::string target(enum_key_name);
    for (const auto& [key, val] : Dasher::Settings::parameter_defaults) {
        if (val.enumKeyName == target) return static_cast<int>(key);
    }
    return -1;
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
    } catch (...) {
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
    s_paramInfoGroup = val.group;
    out->group = s_paramInfoGroup.c_str();
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
    if (ctx->mouseDown) {
        ctx->intf->KeyUp(nowMs(), Dasher::Keys::Primary_Input);
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

// ── Appearance / dark mode (RFC 0007) ──────────────────────────────────────
// (Helpers live above `extern "C"` — they return std::string / use C++ types.)

DASHER_API int dasher_get_palette_appearance(dasher_ctx* ctx, int index) {
    if (!ctx || !ctx->intf) return -1;
    auto colorIO = ctx->intf->GetColorIO();
    if (!colorIO) return -1;
    auto names = ctx->intf->GetPermittedValues(Dasher::SP_COLOUR_ID);
    if (index < 0 || index >= static_cast<int>(names.size())) return -1;
    const Dasher::ColorPalette* p = colorIO->FindPalette(names[index]);
    if (!p || p->PaletteName != names[index]) return 0; // not found -> unspecified
    return static_cast<int>(p->AppearanceValue);
}

DASHER_API const char* dasher_find_companion_palette(dasher_ctx* ctx, const char* palette_name) {
    if (!ctx || !ctx->intf || !palette_name) return nullptr;
    auto colorIO = ctx->intf->GetColorIO();
    if (!colorIO) return nullptr;
    const Dasher::ColorPalette* comp = companionLookup(colorIO, palette_name);
    if (!comp) return nullptr;
    ctx->tlString = comp->PaletteName;
    return ctx->tlString.c_str();
}

DASHER_API int dasher_get_appearance_mode(dasher_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->appearanceMode;
}

DASHER_API void dasher_set_appearance_mode(dasher_ctx* ctx, int mode) {
    if (!ctx || mode < 0 || mode > 2) return;
    ensureAppearanceInitialised(ctx);
    if (ctx->appearanceMode == mode) return;
    ctx->appearanceMode = mode;
    saveAppearanceSettings(ctx);
    resolveAppearance(ctx);
}

DASHER_API int dasher_get_system_appearance(dasher_ctx* ctx) {
    if (!ctx) return 1;
    return ctx->systemAppearance;
}

DASHER_API void dasher_set_system_appearance(dasher_ctx* ctx, int appearance) {
    if (!ctx || (appearance != 1 && appearance != 2)) return;
    ensureAppearanceInitialised(ctx);
    if (ctx->systemAppearance == appearance) return;
    ctx->systemAppearance = appearance;
    // Only matters in SYSTEM mode, but resolve is cheap and keeps state consistent.
    if (ctx->appearanceMode == 0) resolveAppearance(ctx);
}

DASHER_API const char* dasher_get_light_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ensureAppearanceInitialised(ctx);
    ctx->tlString = ctx->lightPalette;
    return ctx->tlString.c_str();
}

DASHER_API const char* dasher_get_dark_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ensureAppearanceInitialised(ctx);
    ctx->tlString = ctx->darkPalette;
    return ctx->tlString.c_str();
}

DASHER_API void dasher_set_light_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    ctx->lightPalette = name;
    saveAppearanceSettings(ctx);
    resolveAppearance(ctx);
}

DASHER_API void dasher_set_dark_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    ctx->darkPalette = name;
    saveAppearanceSettings(ctx);
    resolveAppearance(ctx);
}

DASHER_API void dasher_set_user_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    int eff = effectiveAppearanceValue(ctx);
    if (eff == 1)
        ctx->lightPalette = name;
    else
        ctx->darkPalette = name;

    // Default the other side to the chosen palette's companion if unset, so the
    // user gets a sensible matching variant without configuring both sides.
    std::string& other = (eff == 1) ? ctx->darkPalette : ctx->lightPalette;
    if (other.empty() || other == ctx->lightPalette || other == ctx->darkPalette) {
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, name)) other = comp->PaletteName;
        }
    }
    saveAppearanceSettings(ctx);
    resolveAppearance(ctx);
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

static std::string s_gameTextBuf;

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
    s_gameTextBuf = symbolsToText(gm->GetAlphabet(), syms, (int)syms.size());
    return s_gameTextBuf.c_str();
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
    s_gameTextBuf = gm->GetWrongText();
    return s_gameTextBuf.c_str();
}

// ── Persistence ───────────────────────────────────────────────────────────

DASHER_API void dasher_save_settings(dasher_ctx* ctx) {
    if (!ctx || !ctx->settings) return;
    ctx->settings->Save();
    if (ctx->appearanceLoaded) saveAppearanceSettings(ctx); // RFC 0007 sidecar
}

// ── Localization ──────────────────────────────────────────────────────────

static std::unordered_map<std::string, std::string> parseStringsJson(const std::string& content) {
    std::unordered_map<std::string, std::string> result;
    enum class State { Key, Colon, Value, Skip };
    State state = State::Skip;
    (void)state;
    std::string key, value;
    bool inString = false;
    bool escape = false;
    bool buildingKey = true;
    int depth = 0;

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];

        if (escape) {
            if (buildingKey)
                key += c;
            else
                value += c;
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
            if (buildingKey)
                key += c;
            else
                value += c;
            continue;
        }

        if (c == '{')
            depth++;
        else if (c == '}')
            depth--;
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

DASHER_API void dasher_set_output_callback(dasher_ctx* ctx, dasher_output_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->outputCb = callback;
    ctx->outputCbUserData = user_data;
}

DASHER_API void dasher_set_message_callback(dasher_ctx* ctx, dasher_message_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->messageCb = callback;
    ctx->messageCbUserData = user_data;
}

DASHER_API void dasher_set_speak_callback(dasher_ctx* ctx, dasher_speak_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->speakCb = callback;
    ctx->speakCbUserData = user_data;
}

DASHER_API void dasher_set_clipboard_callback(dasher_ctx* ctx, dasher_clipboard_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->clipboardCb = callback;
    ctx->clipboardCbUserData = user_data;
}

DASHER_API void dasher_set_parameter_callback(dasher_ctx* ctx, dasher_parameter_callback callback, void* user_data) {
    if (!ctx) return;
    ctx->paramCb = callback;
    ctx->paramCbUserData = user_data;
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
        return 0;
    } catch (...) {
        return -1;
    }
}

DASHER_API int dasher_get_offset(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf || !ctx->realized) return -1;
    auto* model = ctx->intf->GetModel();
    if (!model) return -1;
    return model->GetOffset();
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

} // extern "C"
