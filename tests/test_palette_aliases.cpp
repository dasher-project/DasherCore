// Group-alias expansion pin (internal: drives CColorIO/ColorPalette
// directly). Alphabets refer to colour groups by a scattered set of names
// ("lower case letters", "ethiopic letters", "arabic-indic numbers", ...);
// the legacy parser registered aliases for all of them, new-format
// palettes must too or WA alphabets fall back to the palette default.
#include "test_common.h"

#include "DasherCore/ColorIO.h"
#include "DasherCore/ColorPalette.h"
#include "DasherCore/Parameters.h"
#include "DasherCore/SettingsStore.h"

TEST(new_format_palettes_resolve_legacy_group_aliases) {
    Dasher::CSettingsStore store;
    store.AddParameters(Dasher::Settings::parameter_defaults);
    Dasher::CColorIO io(nullptr);

    pugi::xml_document doc;
    ASSERT(doc.load_file(TEST_DATA_DIR "/Data/colours/colour.default.xml"));
    ASSERT(io.Parse(doc, "colour.default.xml", false));

    const Dasher::ColorPalette* pal = io.FindPalette("Default");
    ASSERT(pal != nullptr);
    const auto& groups = pal->GetGroupColorsMap();

    const std::vector<std::pair<std::string, std::string>> alias_of_canonical = {
        {"lowercase", "lower case letters"},
        {"lowercase", "Lower case Latin letters"},
        {"uppercase", "upper case letters"},
        {"uppercase", "Upper case Latin letters"},
        {"punctuation", "Punctuation"},
        {"punctuation", "limitedPunctuation"},
        {"punctuation", "ascii punctuation"},
        {"numbers", "Numbers"},
        {"numbers", "arabic-indic numbers"},
        {"lowercase", "ethiopic letters"},
        {"lowercase", "vowels etc"},
        {"lowercase", "vowel signs etc"},
        {"lowercase", "vowel-like letters"},
        {"lowercase", "character modifiers?"},
        {"lowercase", "hamza"},
        {"lowercase", "joiners"},
    };
    // Groups the file defines itself keep their own (deliberately
    // different) sequences — the expansion only fills what's missing.
    const std::set<std::string> file_defined = {"lowercase",
                                                "lowercaseBackground",
                                                "uppercase",
                                                "punctuation",
                                                "limitedPunctuation",
                                                "punctuationLong",
                                                "numbers",
                                                "accents",
                                                "space",
                                                "paragraph",
                                                "paragraphSpace"};
    for (const auto& [canonical, alias] : alias_of_canonical) {
        ASSERT(groups.count(alias) == 1);
        ASSERT(groups.count(canonical) == 1);
        if (file_defined.count(alias)) continue;
        ASSERT(groups.at(alias).nodeColorSequence == groups.at(canonical).nodeColorSequence);
        ASSERT(groups.at(alias).groupColor == groups.at(canonical).groupColor);
    }

    // Spot-check the v6 palette design (legacy indices 111/112/113 — the
    // group-box colours #62 restored): uppercase yellow, punctuation
    // green, numbers red.
    const auto up = groups.at("upper case letters").groupColor.first;
    const auto pu = groups.at("Punctuation").groupColor.first;
    const auto nu = groups.at("Numbers").groupColor.first;
    printf("  boxes: uppercase #%02x%02x%02x punctuation #%02x%02x%02x numbers #%02x%02x%02x\n", up.Red, up.Green,
           up.Blue, pu.Red, pu.Green, pu.Blue, nu.Red, nu.Green, nu.Blue);
    ASSERT(up.Red == 255 && up.Green == 255 && up.Blue == 0);
    ASSERT(pu.Red == 0 && pu.Green == 200 && pu.Blue == 0);
    ASSERT(nu.Red == 255 && nu.Green == 0 && nu.Blue == 0);
}
