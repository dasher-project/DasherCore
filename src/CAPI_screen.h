#ifndef DASHER_CAPI_SCREEN_H
#define DASHER_CAPI_SCREEN_H

// CAPI_screen.h — the C API's canvas: the command-buffer screen and the
// pointer input device (todo.md Phase 2.3, moved verbatim from CAPI.cpp).
//
// NOT installed, NOT part of the public API/ABI: dasher.h's draw-command
// contract is the public surface; CommandScreen is the engine-side producer
// of that buffer. Both classes are fully inline; only CAPI.cpp includes
// this header (dasher_ctx forward-declares both in CAPI_internal.h and the
// context is only constructed/destroyed in CAPI.cpp).

#include "CAPI_internal.h"

#include "DasherCore/DasherInput.h"
#include "DasherCore/DasherScreen.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ── Command-buffer screen ──────────────────────────────────────────────────

// UTF-8 code-point count (lead-byte count). Used by the fallback width
// estimate: counting bytes over-measured multibyte text ('é' = 2 bytes),
// which made the label shunting over-shove for accented alphabets.
inline int utf8_codepoint_count(const std::string& s) {
    int n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) n++;
    }
    return n;
}

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

    // Frontend-supplied text measurement (issue #56): when present, label
    // layout uses the real metrics of the font the canvas draws with. Stored
    // here (not on the ctx) because TextSize is a screen method.
    void SetTextSizeCallback(dasher_text_size_callback cb, void* userData) {
        m_textSizeCb = cb;
        m_textSizeUserData = userData;
        ++m_metricsGeneration; // new callback = new measurements
    }

    void TextMetricsChanged() { ++m_metricsGeneration; }

    // Label variant that caches measurements per font size, invalidated by a
    // generation counter when the canvas font changes. TextSize is called for
    // every visible label on (potentially) every frame; the callback crosses
    // an FFI boundary and does real font measurement on the frontend side, so
    // caching here keeps steady-state frames callback-free.
    class CachingLabel : public CDasherScreen::Label {
      public:
        CachingLabel(const std::string& strText, unsigned int iWrapSize) : Label(strText, iWrapSize) {}

        struct Measurement {
            uint64_t generation;
            Dasher::screenint width;
            Dasher::screenint height;
        };
        std::unordered_map<unsigned int, Measurement> measured;
    };

    Label* MakeLabel(const std::string& strText, unsigned int iWrapSize) override {
        return new CachingLabel(strText, iWrapSize);
    }

    std::pair<Dasher::screenint, Dasher::screenint> TextSize(Label* label, unsigned int iFontSize) override {
        if (!label) return std::make_pair(Dasher::screenint(0), Dasher::screenint(0));

        // Only single-line labels go through the frontend: wrapped labels
        // (lock/pause message) would need wrap-aware measurement, which this
        // callback contract doesn't express.
        auto* caching = dynamic_cast<CachingLabel*>(label);
        if (caching && caching->m_iWrapSize == 0) {
            auto it = caching->measured.find(iFontSize);
            if (it != caching->measured.end() && it->second.generation == m_metricsGeneration)
                return std::make_pair(it->second.width, it->second.height);

            if (m_textSizeCb) {
                int w = -1, h = -1;
                if (m_textSizeCb(label->m_strText.c_str(), static_cast<int>(iFontSize), &w, &h, m_textSizeUserData) ==
                        0 &&
                    w >= 0 && h >= 0) {
                    auto result = std::make_pair(Dasher::screenint(w), Dasher::screenint(h));
                    caching->measured[iFontSize] = {m_metricsGeneration, result.first, result.second};
                    return result;
                }
                // Callback failed: fall through to the estimate, but don't
                // cache it — a later frame may measure successfully.
            } else {
                // No callback: cache the estimate too, so dynamic_cast-heavy
                // paths behave identically with and without one.
                auto result = EstimateTextSize(label, iFontSize);
                caching->measured[iFontSize] = {m_metricsGeneration, result.first, result.second};
                return result;
            }
        }

        return EstimateTextSize(label, iFontSize);
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

    // ── Cube mode (Options::CUBE) ───────────────────────────────────────────
    // The flat command buffer has no 3D backend, so cube mode renders as flat
    // shaded rectangles: one filled rect (opcode 4) per cube face, plus an
    // outline (opcode 3) when requested. Without these overrides the whole cube
    // path is silently dropped (the base-class methods are no-ops), which left
    // the canvas black — see issue #47.
    void DrawCube(float posX, float posY, float sizeX, float sizeY, Dasher::CubeDepthLevel, Dasher::CubeDepthLevel,
                  const Dasher::ColorPalette::Color& color, const Dasher::ColorPalette::Color& outlineColor,
                  int iThickness) override {
        const int x1 = static_cast<int>(posX - sizeX / 2.0f);
        const int y1 = static_cast<int>(posY - sizeY / 2.0f);
        const int x2 = static_cast<int>(posX + sizeX / 2.0f);
        const int y2 = static_cast<int>(posY + sizeY / 2.0f);
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
        if (iThickness > 0 && !outlineColor.isFullyTransparent()) push(3, x1, y1, x2, y2, colorToARGB(outlineColor));
    }

    void Draw3DLabel(Label* label, Dasher::screenint x, Dasher::screenint y, Dasher::screenint,
                     Dasher::Options::ScreenOrientations, Dasher::myint, Dasher::myint, unsigned int iFontSize,
                     const Dasher::ColorPalette::Color& color) override {
        // Cube-mode labels reach the screen via Draw3DLabel rather than DrawString;
        // emit the same opcode-5 text command. The 3D extrusion is lost, but the
        // label is positioned/coloured identically.
        DrawString(label, x, y, iFontSize, color);
    }

    void DrawProjectedRectangle(Dasher::screenint posX, Dasher::screenint posY, Dasher::screenint sizeX,
                                Dasher::screenint sizeY, const Dasher::ColorPalette::Color& color) override {
        // The cube-mode crosshair bar. Emit it as a filled rectangle.
        const int x1 = static_cast<int>(posX - sizeX / 2);
        const int y1 = static_cast<int>(posY - sizeY / 2);
        const int x2 = static_cast<int>(posX + sizeX / 2);
        const int y2 = static_cast<int>(posY + sizeY / 2);
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
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

    // The built-in estimate, used when no callback is registered (or a
    // measurement failed). Counts UTF-8 code points, not bytes: multibyte
    // text previously measured as 2× its glyph count, over-shoving the label
    // layout for accented alphabets. Still an estimate — real metrics come
    // from the frontend via the text-size callback (issue #56).
    static std::pair<Dasher::screenint, Dasher::screenint> EstimateTextSize(Label* label, unsigned int iFontSize) {
        const int codepoints = utf8_codepoint_count(label->m_strText);
        return std::make_pair(Dasher::screenint(codepoints * iFontSize / 2), Dasher::screenint(iFontSize));
    }

    dasher_text_size_callback m_textSizeCb = nullptr;
    void* m_textSizeUserData = nullptr;
    // Bumped by SetTextSizeCallback / TextMetricsChanged; label caches whose
    // entry carries an older generation are re-measured.
    uint64_t m_metricsGeneration = 0;

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
};#ifndef DASHER_CAPI_SCREEN_H
#define DASHER_CAPI_SCREEN_H

// CAPI_screen.h — the C API's canvas: the command-buffer screen and the
// pointer input device (todo.md Phase 2.3, moved verbatim from CAPI.cpp).
//
// NOT installed, NOT part of the public API/ABI: dasher.h's draw-command
// contract is the public surface; CommandScreen is the engine-side producer
// of that buffer. Both classes are fully inline; only CAPI.cpp includes
// this header (dasher_ctx forward-declares both in CAPI_internal.h and the
// context is only constructed/destroyed in CAPI.cpp).

#include "CAPI_internal.h"

#include "DasherCore/DasherInput.h"
#include "DasherCore/DasherScreen.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ── Command-buffer screen ──────────────────────────────────────────────────

// UTF-8 code-point count (lead-byte count). Used by the fallback width
// estimate: counting bytes over-measured multibyte text ('é' = 2 bytes),
// which made the label shunting over-shove for accented alphabets.
inline int utf8_codepoint_count(const std::string& s) {
    int n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) n++;
    }
    return n;
}

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

    // Frontend-supplied text measurement (issue #56): when present, label
    // layout uses the real metrics of the font the canvas draws with. Stored
    // here (not on the ctx) because TextSize is a screen method.
    void SetTextSizeCallback(dasher_text_size_callback cb, void* userData) {
        m_textSizeCb = cb;
        m_textSizeUserData = userData;
        ++m_metricsGeneration; // new callback = new measurements
    }

    void TextMetricsChanged() { ++m_metricsGeneration; }

    // Label variant that caches measurements per font size, invalidated by a
    // generation counter when the canvas font changes. TextSize is called for
    // every visible label on (potentially) every frame; the callback crosses
    // an FFI boundary and does real font measurement on the frontend side, so
    // caching here keeps steady-state frames callback-free.
    class CachingLabel : public CDasherScreen::Label {
      public:
        CachingLabel(const std::string& strText, unsigned int iWrapSize) : Label(strText, iWrapSize) {}

        struct Measurement {
            uint64_t generation;
            Dasher::screenint width;
            Dasher::screenint height;
        };
        std::unordered_map<unsigned int, Measurement> measured;
    };

    Label* MakeLabel(const std::string& strText, unsigned int iWrapSize) override {
        return new CachingLabel(strText, iWrapSize);
    }

    std::pair<Dasher::screenint, Dasher::screenint> TextSize(Label* label, unsigned int iFontSize) override {
        if (!label) return std::make_pair(Dasher::screenint(0), Dasher::screenint(0));

        // Only single-line labels go through the frontend: wrapped labels
        // (lock/pause message) would need wrap-aware measurement, which this
        // callback contract doesn't express.
        auto* caching = dynamic_cast<CachingLabel*>(label);
        if (caching && caching->m_iWrapSize == 0) {
            auto it = caching->measured.find(iFontSize);
            if (it != caching->measured.end() && it->second.generation == m_metricsGeneration)
                return std::make_pair(it->second.width, it->second.height);

            if (m_textSizeCb) {
                int w = -1, h = -1;
                if (m_textSizeCb(label->m_strText.c_str(), static_cast<int>(iFontSize), &w, &h, m_textSizeUserData) ==
                        0 &&
                    w >= 0 && h >= 0) {
                    auto result = std::make_pair(Dasher::screenint(w), Dasher::screenint(h));
                    caching->measured[iFontSize] = {m_metricsGeneration, result.first, result.second};
                    return result;
                }
                // Callback failed: fall through to the estimate, but don't
                // cache it — a later frame may measure successfully.
            } else {
                // No callback: cache the estimate too, so dynamic_cast-heavy
                // paths behave identically with and without one.
                auto result = EstimateTextSize(label, iFontSize);
                caching->measured[iFontSize] = {m_metricsGeneration, result.first, result.second};
                return result;
            }
        }

        return EstimateTextSize(label, iFontSize);
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

    // ── Cube mode (Options::CUBE) ───────────────────────────────────────────
    // The flat command buffer has no 3D backend, so cube mode renders as flat
    // shaded rectangles: one filled rect (opcode 4) per cube face, plus an
    // outline (opcode 3) when requested. Without these overrides the whole cube
    // path is silently dropped (the base-class methods are no-ops), which left
    // the canvas black — see issue #47.
    void DrawCube(float posX, float posY, float sizeX, float sizeY, Dasher::CubeDepthLevel, Dasher::CubeDepthLevel,
                  const Dasher::ColorPalette::Color& color, const Dasher::ColorPalette::Color& outlineColor,
                  int iThickness) override {
        const int x1 = static_cast<int>(posX - sizeX / 2.0f);
        const int y1 = static_cast<int>(posY - sizeY / 2.0f);
        const int x2 = static_cast<int>(posX + sizeX / 2.0f);
        const int y2 = static_cast<int>(posY + sizeY / 2.0f);
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
        if (iThickness > 0 && !outlineColor.isFullyTransparent()) push(3, x1, y1, x2, y2, colorToARGB(outlineColor));
    }

    void Draw3DLabel(Label* label, Dasher::screenint x, Dasher::screenint y, Dasher::screenint,
                     Dasher::Options::ScreenOrientations, Dasher::myint, Dasher::myint, unsigned int iFontSize,
                     const Dasher::ColorPalette::Color& color) override {
        // Cube-mode labels reach the screen via Draw3DLabel rather than DrawString;
        // emit the same opcode-5 text command. The 3D extrusion is lost, but the
        // label is positioned/coloured identically.
        DrawString(label, x, y, iFontSize, color);
    }

    void DrawProjectedRectangle(Dasher::screenint posX, Dasher::screenint posY, Dasher::screenint sizeX,
                                Dasher::screenint sizeY, const Dasher::ColorPalette::Color& color) override {
        // The cube-mode crosshair bar. Emit it as a filled rectangle.
        const int x1 = static_cast<int>(posX - sizeX / 2);
        const int y1 = static_cast<int>(posY - sizeY / 2);
        const int x2 = static_cast<int>(posX + sizeX / 2);
        const int y2 = static_cast<int>(posY + sizeY / 2);
        if (!color.isFullyTransparent()) push(4, x1, y1, x2, y2, colorToARGB(color));
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

    // The built-in estimate, used when no callback is registered (or a
    // measurement failed). Counts UTF-8 code points, not bytes: multibyte
    // text previously measured as 2× its glyph count, over-shoving the label
    // layout for accented alphabets. Still an estimate — real metrics come
    // from the frontend via the text-size callback (issue #56).
    static std::pair<Dasher::screenint, Dasher::screenint> EstimateTextSize(Label* label, unsigned int iFontSize) {
        const int codepoints = utf8_codepoint_count(label->m_strText);
        return std::make_pair(Dasher::screenint(codepoints * iFontSize / 2), Dasher::screenint(iFontSize));
    }

    dasher_text_size_callback m_textSizeCb = nullptr;
    void* m_textSizeUserData = nullptr;
    // Bumped by SetTextSizeCallback / TextMetricsChanged; label caches whose
    // entry carries an older generation are re-measured.
    uint64_t m_metricsGeneration = 0;

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

#endif // DASHER_CAPI_SCREEN_H
 // DASHER_CAPI_SCREEN_H
