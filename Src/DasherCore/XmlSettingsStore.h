#pragma once

#include <string>
#include <map>

#include "SettingsStore.h"
#include "AbstractXMLParser.h"


namespace Dasher {
// This class is not thread-safe.
class XmlSettingsStore : public Dasher::CSettingsStore, public AbstractXMLParser {
 public:
	XmlSettingsStore(const std::string& filename, CMessageDisplay* pDisplay);
	~XmlSettingsStore() override = default;

	// Load the XML file and fills in the default values needed.
	// Returns true on success.
	void Load();
	// Saves the XML file, returns true on success.
	bool Save();

	bool Parse(pugi::xml_document& document, bool bUser) override;

 private:
	bool LoadSetting(const std::string_view& Key, bool* Value) override;
	bool LoadSetting(const std::string_view& Key, long* Value) override;
	bool LoadSetting(const std::string_view& Key, std::string* Value) override;

	void SaveSetting(const std::string_view& Key, bool Value) override;
	void SaveSetting(const std::string_view& Key, long Value) override;
	void SaveSetting(const std::string_view& Key, const std::string& Value) override;
	
	// Save if the mode is 'SAVE_IMMEDIATELY', otherwise just set 'modified_' to
	// true.
	void SaveIfNeeded();

	enum Mode {
		// Save each time 'SaveSetting' is called.
		SAVE_IMMEDIATELY,
		// Save only when 'Save' is called.
		EXPLICIT_SAVE
	};

	Mode mode_ = EXPLICIT_SAVE;
	std::string filename_;
	bool modified_ = false;
	std::map<std::string, bool> boolean_settings_;
	std::map<std::string, long> long_settings_;
	std::map<std::string, std::string> string_settings_;
};

}  // namespace Dasher