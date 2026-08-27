#include "XmlSettingsStore.h"
#include "DasherInterfaceBase.h"
#include "FileUtils.h"

namespace Dasher {

template <typename T>
static bool Read(const std::map<std::string, T> values, const std::string& key, T* value) {
    auto i = values.find(key);
    if (i == values.end()) {
        return false;
    }
    *value = i->second;
    return true;
}

XmlSettingsStore::XmlSettingsStore(const std::string& filename, CMessageDisplay* pDisplay)
    : AbstractXMLParser(pDisplay), last_mutable_filepath(filename) {}

void XmlSettingsStore::RefreshFromStore() {
    // Save the current parsed maps so we can restore them if the file is
    // missing, unreadable, or malformed — clearing upfront and parsing into
    // nothing would make LoadPersistent treat every persistent setting as
    // removed and reset the running engine to defaults (mass callbacks and
    // rebuilds for what is a transient I/O problem, not a real removal).
    const auto saved_bool = boolean_settings_;
    const auto saved_long = long_settings_;
    const auto saved_string = string_settings_;

    // Clear the parsed maps so entries removed from the file are actually
    // removed (Parse only inserts/overwrites — stale keys would survive a
    // re-parse and LoadPersistent would treat them as still present,
    // keeping the old value instead of restoring the default).
    boolean_settings_.clear();
    long_settings_.clear();
    string_settings_.clear();

    // Re-parse the XML file into the settings maps, then repopulate
    // parameters_ from those maps (same path Load() uses, but without
    // the mode switching). The parse overwrites the maps in place;
    // LoadPersistent then updates parameters_ entries from them.
    // reload_parse_ok_ is set by Parse() — it is only reached when the
    // document actually loaded (missing/unreadable/malformed files fail
    // earlier), which distinguishes a real empty-file removal from an
    // I/O failure.
    reload_parse_ok_ = false;
    Dasher::FileUtils::ScanFiles(this, last_mutable_filepath);

    // If the file didn't parse (missing/unreadable/malformed), keep the
    // previous state entirely — maps AND parameters. A transient I/O
    // problem must not reset the running engine to defaults, so skip
    // LoadPersistent too: ReloadFromFile's diff then sees no change and
    // emits no notifications.
    if (!reload_parse_ok_) {
        boolean_settings_ = saved_bool;
        long_settings_ = saved_long;
        string_settings_ = saved_string;
        return;
    }

    mode_ = EXPLICIT_SAVE;
    LoadPersistent();
    mode_ = SAVE_IMMEDIATELY;
}

void XmlSettingsStore::Load() {
    Dasher::FileUtils::ScanFiles(this, last_mutable_filepath);
    // Load all the settings or create defaults for the ones that don't exist.
    // The superclass 'ParseFile' saves default settings if not found.
    mode_ = EXPLICIT_SAVE;
    LoadPersistent();
    mode_ = SAVE_IMMEDIATELY;
}

bool XmlSettingsStore::LoadSetting(const std::string& key, bool* value) {
    return Read(boolean_settings_, key, value);
}

bool XmlSettingsStore::LoadSetting(const std::string& key, long* value) {
    return Read(long_settings_, key, value);
}

bool XmlSettingsStore::LoadSetting(const std::string& key, std::string* value) {
    return Read(string_settings_, key, value);
}

void XmlSettingsStore::SaveSetting(const std::string& key, bool value) {
    boolean_settings_[key] = value;
    SaveIfNeeded();
}

void XmlSettingsStore::SaveSetting(const std::string& key, long value) {
    long_settings_[key] = value;
    SaveIfNeeded();
}

void XmlSettingsStore::SaveSetting(const std::string& key, const std::string& value) {
    string_settings_[key] = value;
    SaveIfNeeded();
}

void XmlSettingsStore::SaveIfNeeded() {
    modified_ = true;
    if (mode_ == SAVE_IMMEDIATELY) {
        Save();
    }
}

bool XmlSettingsStore::Save() {
    if (!modified_) {
        return true;
    }

    modified_ = false;

    pugi::xml_document doc;
    pugi::xml_node declaration_node = doc.append_child(pugi::node_declaration);
    declaration_node.append_attribute("version") = "1.0";
    declaration_node.append_attribute("encoding") = "UTF-8";
    declaration_node.append_attribute("standalone") = "no";

    pugi::xml_node doctype_node = doc.append_child(pugi::node_doctype);
    doctype_node.set_value("settings SYSTEM \"settings.dtd\"");

    pugi::xml_node settings = doc.append_child("settings");

    for (const auto& [name, value] : long_settings_) {
        pugi::xml_node long_node = settings.append_child("long");
        long_node.append_attribute("name") = name.c_str();
        long_node.append_attribute("value") = value;
    }

    for (const auto& [name, value] : boolean_settings_) {
        pugi::xml_node bool_node = settings.append_child("bool");
        bool_node.append_attribute("name") = name.c_str();
        bool_node.append_attribute("value") = value;
    }

    for (const auto& [name, value] : string_settings_) {
        pugi::xml_node string_node = settings.append_child("string");
        string_node.append_attribute("name") = name.c_str();
        string_node.append_attribute("value") = value.c_str();
    }

    return doc.save_file(last_mutable_filepath.c_str(), "\t", pugi::format_default, pugi::encoding_utf8);
}

bool XmlSettingsStore::Parse(pugi::xml_document& document, const std::string filePath, bool bUser) {
    if (bUser) last_mutable_filepath = filePath;

    const pugi::xml_node outer = document.child("settings");

    // A settings file must have a <settings> root. Well-formed XML with
    // any other root is not a settings file — marking the reload
    // successful would leave the maps empty and LoadPersistent would
    // reset the engine to defaults.
    if (!outer) {
        reload_parse_ok_ = false;
        return false;
    }
    reload_parse_ok_ = true; // reached only when the document loaded
    for (pugi::xml_node bool_setting : outer.children("bool")) {
        std::string name = bool_setting.attribute("name").as_string();
        const bool value = bool_setting.attribute("value").as_bool();
        if (!name.empty()) boolean_settings_[name] = value;
    }

    for (pugi::xml_node string_setting : outer.children("string")) {
        std::string name = string_setting.attribute("name").as_string();
        const std::string value = string_setting.attribute("value").as_string();
        if (!name.empty()) string_settings_[name] = value;
    }

    for (pugi::xml_node long_setting : outer.children("long")) {
        std::string name = long_setting.attribute("name").as_string();
        const long value = static_cast<long>(long_setting.attribute("value").as_llong());
        if (!name.empty()) long_settings_[name] = value;
    }

    return true;
}
} // namespace Dasher
