// CAPI_params.cpp — parameter introspection: the self-describing settings
// schema and the language-model registry accessors (todo.md Phase 2.6,
// moved verbatim from CAPI.cpp).
//
// The engine's parameter manifest (Dasher::Settings::parameter_defaults) is
// static, so these tables are process-global by design (unlike per-context
// values, which live behind the typed get/set parameter functions in
// CAPI.cpp). Localized names/descriptions come from the locale tables in
// CAPI_locale.cpp via the capi:: accessors.

#include "CAPI_internal.h"

#include "DasherCore/LanguageModelling/LMRegistry.h"
#include "DasherCore/Parameters.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
// Static schema data. The scratch strings back the dasher_parameter_info
// name/desc/group pointers ("valid until next call" — the header documents
// this per-function).
std::vector<Dasher::Parameter> s_paramKeys;
std::string s_paramInfoName;
std::string s_paramInfoDesc;
std::string s_paramInfoGroup;
std::string s_paramInfoSubgroup;
std::vector<std::string> s_enumStrings;

void ensureParamKeys() {
    if (!s_paramKeys.empty()) return;
    for (const auto& [key, val] : Dasher::Settings::parameter_defaults) {
        s_paramKeys.push_back(key);
    }
    std::sort(s_paramKeys.begin(), s_paramKeys.end());
}
} // namespace

extern "C" {

// ── Language-model registry (static: no ctx) ──────────────────────────────

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
    if (!desc) return "";
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

// ── Parameter schema introspection ─────────────────────────────────────────

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

    // Bind each table once: the iterator comparisons are only valid
    // because the accessors return references to process-global tables.
    const auto& overrides = capi::overrideStrings();
    const auto& locale = capi::localeStrings();
    std::string nameKey = val.enumKeyName + ".label";
    auto nameIt = overrides.find(nameKey);
    if (nameIt != overrides.end()) {
        s_paramInfoName = nameIt->second;
    } else {
        nameIt = locale.find(nameKey);
        if (nameIt != locale.end()) {
            s_paramInfoName = nameIt->second;
        } else {
            s_paramInfoName = val.humanName.empty() ? val.storageName : val.humanName;
        }
    }
    out->name = s_paramInfoName.c_str();

    std::string descKey = val.enumKeyName + ".description";
    auto descIt = overrides.find(descKey);
    if (descIt != overrides.end()) {
        s_paramInfoDesc = descIt->second;
    } else {
        descIt = locale.find(descKey);
        if (descIt != locale.end()) {
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
    if (!ctx || !ctx->intf) return 0;

    // Through the shared memoization cache (todo.md 4.3): returned pointers
    // address the cached vector — valid at least until the next API call,
    // like every string return in this API.
    const auto& values = capi::permittedValues(ctx, static_cast<Dasher::Parameter>(key));

    // Probe call (null buffer / zero capacity): return the full count so
    // callers can size a buffer and call again. Previously this returned 0
    // before ever querying the engine, so every permitted-value list (e.g.
    // the 622 alphabets) came back empty and frontends rendered blank
    // pickers.
    if (!out_names || max_out <= 0) return static_cast<int>(values.size());

    int count = static_cast<int>(values.size());
    if (count > max_out) count = max_out;
    for (int i = 0; i < count; i++) {
        out_names[i] = values[i].c_str();
    }
    return count;
}

} // extern "C"
