// test_locales_sync.cpp — guard that Strings/locales.json (the canonical locale
// list, RFC 0003) stays in lock-step with the strings_*.json files actually
// shipped. Prevents silent drift: add a strings_xx.json but forget locales.json
// (or vice versa) and this test fails.
//
// Self-contained: no JSON dependency. Codes are pulled from the
// strings_<code>.json filenames and from a focused scan of locales.json (a
// generated, controlled file whose "code" keys are the only `"code":` matches).

#include "test_common.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static fs::path strings_dir() {
    return fs::path(get_test_data_dir()) / "Strings";
}

// Codes implied by the shipped strings_<code>.json files.
static std::set<std::string> shipped_codes() {
    std::set<std::string> codes;
    for (const auto& entry : fs::directory_iterator(strings_dir())) {
        const std::string name = entry.path().filename().string();
        const std::string pfx = "strings_";
        const std::string ext = ".json";
        if (name.rfind(pfx, 0) == 0 && name.size() > pfx.size() + ext.size() &&
            name.compare(name.size() - ext.size(), ext.size(), ext) == 0) {
            codes.insert(name.substr(pfx.size(), name.size() - pfx.size() - ext.size()));
        }
    }
    return codes;
}

// Codes listed in locales.json, by scanning for `"code": "<value>"`.
static std::set<std::string> listed_codes() {
    const fs::path file = strings_dir() / "locales.json";
    std::ifstream in(file);
    INFO("could not open " << file.string());
    REQUIRE(in.good());
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();
    const std::string needle = "\"code\":";
    std::set<std::string> codes;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        pos += needle.size();
        const size_t q1 = text.find('"', pos);
        if (q1 == std::string::npos) break;
        const size_t q2 = text.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        codes.insert(text.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }
    return codes;
}

TEST_CASE("locales.json matches the shipped strings_*.json files") {
    const std::set<std::string> shipped = shipped_codes();
    const std::set<std::string> listed = listed_codes();

    REQUIRE_FALSE(shipped.empty());
    MESSAGE("shipped=" << shipped.size() << " listed=" << listed.size());

    std::set<std::string> missing; // shipped but not listed
    std::set<std::string> extra;   // listed but not shipped
    std::set_difference(shipped.begin(), shipped.end(), listed.begin(), listed.end(),
                        std::inserter(missing, missing.begin()));
    std::set_difference(listed.begin(), listed.end(), shipped.begin(), shipped.end(),
                        std::inserter(extra, extra.begin()));

    INFO("locales.json missing codes that have a strings_*.json file (missing=" << missing.size() << ")");
    REQUIRE(missing.empty());
    INFO("locales.json lists codes with no strings_*.json file (extra=" << extra.size() << ")");
    REQUIRE(extra.empty());
}
