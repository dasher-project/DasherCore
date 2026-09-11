// CAPI_appearance.cpp — the RFC 0007 light/dark appearance model (todo.md
// Phase 2.4, moved verbatim from CAPI.cpp).
//
// Owns: mode (system/light/dark), transient OS appearance input, the two
// persisted palette preferences, companion lookup, and the sidecar file
// <userDir>/appearance_settings.xml. The active palette (SP_COLOUR_ID) is
// always DERIVED via capi::resolveAppearance, so an auto-switch can never
// overwrite the user's explicit choice.
//
// capi::loadAppearanceSettings / saveAppearanceSettings / resolveAppearance
// are shared with CAPI.cpp (dasher_create, dasher_save_settings) and are
// declared in CAPI_internal.h; everything else is TU-local.

#include "CAPI_internal.h"

#include "DasherCore/ColorIO.h"
#include "pugixml.hpp"

#include <string>

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
    if (ctx->appearance.mode == 1) return 1; // forced light
    if (ctx->appearance.mode == 2) return 2; // forced dark
    return ctx->appearance.systemAppearance;           // follow system (defaults to light)
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

// Ensure the model is initialised: on first use, seed the light preference from
// the engine's current palette and default the dark side to its companion.
void ensureAppearanceInitialised(dasher_ctx* ctx) {
    if (ctx->appearance.loaded) {
        capi::resolveAppearance(ctx);
        return;
    }
    capi::loadAppearanceSettings(ctx);
    if (ctx->appearance.lightPalette.empty() && ctx->appearance.darkPalette.empty()) {
        // Fresh start: adopt whatever palette the engine loaded as the light
        // preference, and default the dark side to its companion.
        std::string current = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
        ctx->appearance.lightPalette = current;
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, current))
                ctx->appearance.darkPalette = comp->PaletteName;
        }
        capi::saveAppearanceSettings(ctx);
    }
    capi::resolveAppearance(ctx);
}
} // namespace

// Recompute the active palette from mode + system + preferences and write it to
// SP_COLOUR_ID (what the canvas renders). The persisted preferences are the
// source of truth, so this can never clobber the user's explicit choice.
DASHER_LOCAL void capi::resolveAppearance(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return;

    // Late seed: ensureAppearanceInitialised may have run before Realize,
    // when ColorIO wasn't available and the companion lookup failed.
    // Retry now — once ColorIO exists, the lookup succeeds and fills the gap.
    if (ctx->appearance.darkPalette.empty() && !ctx->appearance.lightPalette.empty()) {
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, ctx->appearance.lightPalette))
                ctx->appearance.darkPalette = comp->PaletteName;
        }
    }

    int eff = effectiveAppearanceValue(ctx);
    std::string target = (eff == 1) ? ctx->appearance.lightPalette : ctx->appearance.darkPalette;
    if (target.empty()) target = (eff == 1) ? ctx->appearance.darkPalette : ctx->appearance.lightPalette; // other side
    if (target.empty()) return; // nothing chosen yet; leave the engine default

    std::string current = ctx->intf->GetStringParameter(Dasher::SP_COLOUR_ID);
    if (current != target) ctx->intf->SetStringParameter(Dasher::SP_COLOUR_ID, target);
}

// Load mode + light/dark preferences from the sidecar. Non-fatal on any error.
DASHER_LOCAL void capi::loadAppearanceSettings(dasher_ctx* ctx) {
    if (ctx->appearance.loaded) return;
    ctx->appearance.loaded = true;
    std::string path = appearanceSettingsPath(ctx);
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(path.c_str());
    if (!res) return; // missing/unreadable: leave defaults
    pugi::xml_node root = doc.child("appearance");
    if (!root) return;
    ctx->appearance.mode = root.attribute("mode").as_int(0);
    ctx->appearance.lightPalette = root.attribute("light").as_string("");
    ctx->appearance.darkPalette = root.attribute("dark").as_string("");
    if (ctx->appearance.mode < 0 || ctx->appearance.mode > 2) ctx->appearance.mode = 0;
}

// Persist mode + light/dark preferences to the sidecar. Non-fatal on any error.
DASHER_LOCAL void capi::saveAppearanceSettings(dasher_ctx* ctx) {
    if (!ctx || ctx->userDir.empty()) return;
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("appearance");
    root.append_attribute("mode") = ctx->appearance.mode;
    root.append_attribute("light") = ctx->appearance.lightPalette.c_str();
    root.append_attribute("dark") = ctx->appearance.darkPalette.c_str();
    doc.save_file(appearanceSettingsPath(ctx).c_str());
}

extern "C" {

// ── Appearance / dark mode (RFC 0007) ──────────────────────────────────────

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
    ctx->scratch.tlString = comp->PaletteName;
    return ctx->scratch.tlString.c_str();
}

DASHER_API int dasher_get_appearance_mode(dasher_ctx* ctx) {
    if (!ctx) return 0;
    return ctx->appearance.mode;
}

DASHER_API void dasher_set_appearance_mode(dasher_ctx* ctx, int mode) {
    if (!ctx || mode < 0 || mode > 2) return;
    ensureAppearanceInitialised(ctx);
    if (ctx->appearance.mode == mode) return;
    ctx->appearance.mode = mode;
    capi::saveAppearanceSettings(ctx);
    capi::resolveAppearance(ctx);
}

DASHER_API int dasher_get_system_appearance(dasher_ctx* ctx) {
    if (!ctx) return 1;
    return ctx->appearance.systemAppearance;
}

DASHER_API void dasher_set_system_appearance(dasher_ctx* ctx, int appearance) {
    if (!ctx || (appearance != 1 && appearance != 2)) return;
    ensureAppearanceInitialised(ctx);
    if (ctx->appearance.systemAppearance == appearance) return;
    ctx->appearance.systemAppearance = appearance;
    // Only matters in SYSTEM mode, but resolve is cheap and keeps state consistent.
    if (ctx->appearance.mode == 0) capi::resolveAppearance(ctx);
}

DASHER_API const char* dasher_get_light_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ensureAppearanceInitialised(ctx);
    ctx->scratch.tlString = ctx->appearance.lightPalette;
    return ctx->scratch.tlString.c_str();
}

DASHER_API const char* dasher_get_dark_palette(dasher_ctx* ctx) {
    if (!ctx || !ctx->intf) return "";
    ensureAppearanceInitialised(ctx);
    ctx->scratch.tlString = ctx->appearance.darkPalette;
    return ctx->scratch.tlString.c_str();
}

DASHER_API void dasher_set_light_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    ctx->appearance.lightPalette = name;
    capi::saveAppearanceSettings(ctx);
    capi::resolveAppearance(ctx);
}

DASHER_API void dasher_set_dark_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    ctx->appearance.darkPalette = name;
    capi::saveAppearanceSettings(ctx);
    capi::resolveAppearance(ctx);
}

DASHER_API void dasher_set_user_palette(dasher_ctx* ctx, const char* name) {
    if (!ctx || !ctx->intf || !name) return;
    ensureAppearanceInitialised(ctx);
    int eff = effectiveAppearanceValue(ctx);
    if (eff == 1)
        ctx->appearance.lightPalette = name;
    else
        ctx->appearance.darkPalette = name;

    // Default the other side to the chosen palette's companion if unset, so the
    // user gets a sensible matching variant without configuring both sides.
    std::string& other = (eff == 1) ? ctx->appearance.darkPalette : ctx->appearance.lightPalette;
    if (other.empty() || other == ctx->appearance.lightPalette || other == ctx->appearance.darkPalette) {
        if (auto* colorIO = ctx->intf->GetColorIO()) {
            if (const Dasher::ColorPalette* comp = companionLookup(colorIO, name)) other = comp->PaletteName;
        }
    }
    capi::saveAppearanceSettings(ctx);
    capi::resolveAppearance(ctx);
}

} // extern "C"
