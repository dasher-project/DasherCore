#include "XmlSettingsStore.h"
#include "DasherInterfaceBase.h"
#include "FileUtils.h"


namespace Dasher {

template <typename T>
static bool Read(const std::map<std::string, T> values, const std::string& key,
                 T* value)
{
	auto i = values.find(key);
	if (i == values.end())
	{
		return false;
	}
	*value = i->second;
	return true;
}

XmlSettingsStore::XmlSettingsStore(const std::string& filename, CMessageDisplay* pDisplay)
	: AbstractXMLParser(pDisplay), filename_(filename)
{
}

void XmlSettingsStore::Load()
{
	Dasher::FileUtils::ScanFiles(this, filename_);
	// Load all the settings or create defaults for the ones that don't exist.
	// The superclass 'ParseFile' saves default settings if not found.
	mode_ = EXPLICIT_SAVE;
	LoadPersistent();
	mode_ = SAVE_IMMEDIATELY;
}

bool XmlSettingsStore::LoadSetting(const std::string_view& key, bool* value)
{
	return Read(boolean_settings_, std::string(key), value);
}

bool XmlSettingsStore::LoadSetting(const std::string_view& key, long* value)
{
	return Read(long_settings_, std::string(key), value);
}

bool XmlSettingsStore::LoadSetting(const std::string_view& key, std::string* value)
{
	return Read(string_settings_, std::string(key), value);
}

void XmlSettingsStore::SaveSetting(const std::string_view& key, bool value)
{
	boolean_settings_[std::string(key)] = value;
	SaveIfNeeded();
}

void XmlSettingsStore::SaveSetting(const std::string_view& key, long value)
{
	long_settings_[std::string(key)] = value;
	SaveIfNeeded();
}

void XmlSettingsStore::SaveSetting(const std::string_view& key,
                                   const std::string& value)
{
	string_settings_[std::string(key)] = value;
	SaveIfNeeded();
}

void XmlSettingsStore::SaveIfNeeded()
{
	modified_ = true;
	if (mode_ == SAVE_IMMEDIATELY)
	{
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

    for (auto &[name, value] : long_settings_) {
		pugi::xml_node long_node = settings.append_child("long");
		long_node.append_attribute("name") = name.data();
		long_node.append_attribute("value") = value;
    }

	for (auto &[name, value] : boolean_settings_) {
		pugi::xml_node bool_node = settings.append_child("bool");
		bool_node.append_attribute("name") = name.data();
		bool_node.append_attribute("value") = value;
    }

	for (auto &[name, value] : string_settings_) {
		pugi::xml_node string_node = settings.append_child("string");
		string_node.append_attribute("name") = name.data();
		string_node.append_attribute("value") = value.c_str();
    }

	std::ostringstream outputstream;
	doc.save(outputstream, "\t", pugi::format_default, pugi::encoding_utf8);

	return FileUtils::WriteUserDataFile(filename_, outputstream.str(), false);
}

bool XmlSettingsStore::Parse(pugi::xml_document& document, bool bUser)
{
	pugi::xml_node outer = document.child("settings");
	for (pugi::xml_node bool_setting : outer.children("bool"))
    {
		std::string name = bool_setting.attribute("name").as_string();
		bool value = bool_setting.attribute("value").as_bool();
		if(!name.empty()) boolean_settings_[name] = value;
	}

	for (pugi::xml_node string_setting : outer.children("string"))
    {
		std::string name = string_setting.attribute("name").as_string();
		std::string value = string_setting.attribute("value").as_string();
		if(!name.empty()) string_settings_[name] = value;
	}

	for (pugi::xml_node long_setting : outer.children("long"))
    {
		std::string name = long_setting.attribute("name").as_string();
		long value = static_cast<long>(long_setting.attribute("value").as_llong());
		if(!name.empty()) long_settings_[name] = value;
	}

	return true;
}
}  // namespace Dasher
