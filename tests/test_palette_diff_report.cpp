// One-off audit (not registered in CMake's test list): parse the legacy
// set (colours/ files with <colours> roots) and the modern set (colors/)
// separately, then diff every name-matched pair — named colours, group
// colour sequences, and group membership. Drives the colours/
// consolidation: which pairs are faithful ports, which drifted, which
// groups exist only on one side.
#include "test_common.h"

#include "DasherCore/ColorIO.h"
#include "DasherCore/ColorPalette.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/SettingsStore.h"

#include <cstdio>
#include <filesystem>
#include <map>
#include <set>
#include <string>

using namespace Dasher;

namespace {

std::string hex(const ColorPalette::Color& c) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02x%02x%02x", c.Red, c.Green, c.Blue);
    return buf;
}

bool seqEqual(const std::vector<ColorPalette::Color>& a, const std::vector<ColorPalette::Color>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void loadDir(CColorIO& io, const std::string& dir, bool legacyRoot) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        pugi::xml_document doc;
        if (!doc.load_file(entry.path().c_str())) continue;
        const char* root = doc.document_element().name();
        const bool isLegacy = std::strcmp(root, "colours") == 0;
        if (isLegacy != legacyRoot) continue;
        io.Parse(doc, entry.path().filename().string(), false);
    }
    io.RelinkParents();
}

} // namespace

TEST(colours_consolidation_diff_report) {
    const std::string data = std::string(TEST_DATA_DIR) + "/Data";
    CColorIO legacy{nullptr};
    CColorIO modern{nullptr};
    loadDir(legacy, data + "/colours", true);
    loadDir(modern, data + "/colors", false);

    const auto* lm = legacy.GetKnownPalettes();
    const auto* mm = modern.GetKnownPalettes();

    for (const auto& [name, mpal] : *mm) {
        auto it = lm->find(name);
        if (it == lm->end()) {
            printf("\n== %s: NO legacy twin (modern-only)\n", name.c_str());
            continue;
        }
        const ColorPalette* lpal = it->second;
        printf("\n== %s\n", name.c_str());

        // Named (chrome) colours
        for (const auto& [key, mc] : mpal->GetNamedColorsMap()) {
            const auto& lc = lpal->GetNamedColor(key);
            if (!(lc == mc)) {
                printf("  named %-28s legacy=%s modern=%s%s\n", key.c_str(), hex(lc).c_str(), hex(mc).c_str(),
                       (lc == ColorPalette::undefinedColor) ? " (legacy MISSING)" : "");
            }
        }

        // Groups
        const auto& mgroups = mpal->GetGroupColorsMap();
        const auto& lgroups = lpal->GetGroupColorsMap();
        std::set<std::string> names;
        for (const auto& [g, _] : mgroups)
            names.insert(g);
        for (const auto& [g, _] : lgroups)
            names.insert(g);
        for (const auto& g : names) {
            const bool hasM = mgroups.count(g) > 0, hasL = lgroups.count(g) > 0;
            if (!hasL) {
                printf("  group %-22s legacy MISSING (modern defines)\n", g.c_str());
                continue;
            }
            if (!hasM) {
                printf("  group %-22s modern MISSING (legacy defines)\n", g.c_str());
                continue;
            }
            const auto& lg = lgroups.at(g);
            const auto& mg = mgroups.at(g);
            if (!seqEqual(lg.nodeColorSequence, mg.nodeColorSequence)) {
                printf("  group %-22s nodeSequence differs (legacy %zu vs modern %zu colours; "
                       "legacy[0..2]=%s,%s,%s modern[0..2]=%s,%s,%s)\n",
                       g.c_str(), lg.nodeColorSequence.size(), mg.nodeColorSequence.size(),
                       lg.nodeColorSequence.size() > 0 ? hex(lg.nodeColorSequence[0]).c_str() : "-",
                       lg.nodeColorSequence.size() > 1 ? hex(lg.nodeColorSequence[1]).c_str() : "-",
                       lg.nodeColorSequence.size() > 2 ? hex(lg.nodeColorSequence[2]).c_str() : "-",
                       mg.nodeColorSequence.size() > 0 ? hex(mg.nodeColorSequence[0]).c_str() : "-",
                       mg.nodeColorSequence.size() > 1 ? hex(mg.nodeColorSequence[1]).c_str() : "-",
                       mg.nodeColorSequence.size() > 2 ? hex(mg.nodeColorSequence[2]).c_str() : "-");
            }
            if (!seqEqual(lg.altNodeColorSequence, mg.altNodeColorSequence)) {
                printf("  group %-22s altSequence differs (legacy %zu vs modern %zu)\n", g.c_str(),
                       lg.altNodeColorSequence.size(), mg.altNodeColorSequence.size());
            }
        }
    }

    // Legacy-only palettes
    for (const auto& [name, _] : *lm) {
        if (mm->find(name) == mm->end()) printf("\n== %s: NO modern twin (legacy-only)\n", name.c_str());
    }
    printf("\n(report ends)\n");
}
