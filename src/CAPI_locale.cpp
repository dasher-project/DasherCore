// CAPI_locale.cpp — localization: locale loading, per-key overrides and
// lookup (todo.md Phase 2.5, moved verbatim from CAPI.cpp).
//
// Locale state is PROCESS-GLOBAL by design today: dasher_set_locale takes
// a ctx but the tables are shared across all contexts — pinned as a known
// wart by test_capi_contracts.cpp; todo.md 5.2 tracks making it
// per-context (a versioned, deliberate change).
//
// The tables are also read by parameter introspection (localized names /
// descriptions in dasher_get_parameter_info, CAPI_params.cpp), hence the
// capi:: accessors declared in CAPI_internal.h.

#include "CAPI_internal.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {
// Process-global locale state (see file header).
std::string s_localeCode = "en";
std::unordered_map<std::string, std::string> s_localeStrings;
std::unordered_map<std::string, std::string> s_overrideStrings;
} // namespace

const std::unordered_map<std::string, std::string>& capi::localeStrings() {
    return s_localeStrings;
}
const std::unordered_map<std::string, std::string>& capi::overrideStrings() {
    return s_overrideStrings;
}

namespace {
// Parse a flat { "key": "value", ... } strings file. Deliberately minimal
// (depth-1 string pairs only) — a real JSON parser replaces it in todo.md
// Phase 5.1. The strings_*.json format is exactly this shape.
std::unordered_map<std::string, std::string> parseStringsJson(const std::string& content) {
    std::unordered_map<std::string, std::string> result;
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
} // namespace

extern "C" {

// ── Localization ──────────────────────────────────────────────────────────

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
    ctx->scratch.stringBuf = s_localeCode;
    return ctx->scratch.stringBuf.c_str();
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
        ctx->scratch.stringBuf = it->second;
        return ctx->scratch.stringBuf.c_str();
    }
    it = s_localeStrings.find(key);
    if (it != s_localeStrings.end()) {
        ctx->scratch.stringBuf = it->second;
        return ctx->scratch.stringBuf.c_str();
    }
    return nullptr;
}

} // extern "C"
