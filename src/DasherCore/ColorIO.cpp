#include "ColorIO.h"
#include <algorithm>
#include <string>
#include <cstring>

using namespace Dasher;

// TODO: Share information with AlphIO class?
CColorIO::CColorIO(CMessageDisplay *pMsgs) : AbstractXMLParser(pMsgs) {
	CreateDefault();
}

CColorIO::~CColorIO()
{
	KnownPalettes.clear();
	delete HardcodedDefaultPalette;
}

void CColorIO::GetKnownPalettes(std::vector<std::string>* ColourList) const {
	ColourList->clear();

	for(auto& [ID, Palette] : KnownPalettes){
		ColourList->push_back(ID);
	}
}

const std::map<std::string, ColorPalette *>* CColorIO::GetKnownPalettes() const {
	return &KnownPalettes;
}

const ColorPalette* CColorIO::FindPalette(const std::string& ColorPaletteName) {
	if(ColorPaletteName.empty()){ // return Default if no colour scheme is specified
		return HardcodedDefaultPalette;
	}

	//count acts like contains(key)
	if(KnownPalettes.count(ColorPaletteName)) {
		return KnownPalettes[ColorPaletteName];
	}

	// if we don't have the colour scheme they asked for, return default
	return HardcodedDefaultPalette;
}

ColorPalette::Color CColorIO::GetAttributeAsColor(pugi::xml_attribute attribute, ColorPalette::Color defaultColor)
{
    return {attribute.as_string(), defaultColor};
}

std::vector<ColorPalette::Color> CColorIO::GetAttributeAsColorList(pugi::xml_attribute attribute, ColorPalette::Color defaultColor)
{
    if(attribute.empty()) return {};

	const std::string_view colorDef = std::string_view(attribute.as_string());
	std::vector<ColorPalette::Color> result;

	std::string_view::const_iterator lastStart = colorDef.begin();
	for(std::string_view::const_iterator i = colorDef.begin(); i < colorDef.end(); ++i)
	{
	    if(*i == ',')
	    {
			result.push_back(ColorPalette::Color(std::string(lastStart, i), defaultColor));
			lastStart = i + 1;
	    }
	}
	result.push_back(ColorPalette::Color(std::string(lastStart, colorDef.end()), defaultColor));

	return result;
}

bool CColorIO::ParseLegacy(pugi::xml_document& document)
{
    pugi::xml_node coloursRoot = document.document_element();
    if(strcmp(coloursRoot.name(), "colours") != 0) return false;

    for(pugi::xml_node paletteNode = coloursRoot.child("palette"); paletteNode;
        paletteNode = paletteNode.next_sibling("palette"))
    {
        const char* paletteName = paletteNode.attribute("name").as_string();
        if(!paletteName[0]) continue;

        std::vector<ColorPalette::Color> colors;
        for(pugi::xml_node c = paletteNode.child("colour"); c; c = c.next_sibling("colour"))
        {
            colors.emplace_back(
                c.attribute("r").as_int(),
                c.attribute("g").as_int(),
                c.attribute("b").as_int(),
                255);
        }

        auto safeColor = [&](int idx) -> ColorPalette::Color {
            if(idx >= 0 && idx < (int)colors.size()) return colors[idx];
            return ColorPalette::Color(0, 0, 0, 255);
        };

        std::unordered_map<NamedColor::knownColorName, ColorPalette::Color> NamedColors = {
            {NamedColor::background,       safeColor(0)},
            {NamedColor::inputLine,        safeColor(1)},
            {NamedColor::inputPosition,    safeColor(2)},
            {NamedColor::defaultOutline,   safeColor(3)},
            {NamedColor::defaultLabel,     safeColor(4)},
            {NamedColor::crosshair,        safeColor(5)},
            {NamedColor::rootNode,         safeColor(7)},
            {NamedColor::conversionNode,   safeColor(8)},
        };

        std::unordered_map<std::string, ColorPalette::GroupColorInfo> GroupColors;

        // Space / paragraphSpace (index 9)
        {
            ColorPalette::GroupColorInfo spaceGroup;
            spaceGroup.groupColor.first = safeColor(9);
            spaceGroup.groupColor.second = safeColor(139);
            spaceGroup.nodeColorSequence.push_back(safeColor(9));
            spaceGroup.altNodeColorSequence.push_back(safeColor(139));
            spaceGroup.nodeOutlineColorSequence = {NamedColors[NamedColor::defaultOutline]};
            spaceGroup.nodeLabelColorSequence = {NamedColors[NamedColor::defaultLabel]};
            spaceGroup.groupOutlineColor = {NamedColors[NamedColor::defaultOutline], ColorPalette::undefinedColor};
            spaceGroup.groupLabelColor = {NamedColors[NamedColor::defaultLabel], ColorPalette::undefinedColor};
            GroupColors["space"] = spaceGroup;
            GroupColors["paragraphSpace"] = spaceGroup;
        }

        // Letter groups: 10-109, alt: 140-239
        // Map all common colorInfoName values used by alphabets
        {
            ColorPalette::GroupColorInfo letterGroup;
            for(int i = 10; i < 110 && i < (int)colors.size(); i++)
                letterGroup.nodeColorSequence.push_back(safeColor(i));
            for(int i = 140; i < 240 && i < (int)colors.size(); i++)
                letterGroup.altNodeColorSequence.push_back(safeColor(i));
            letterGroup.nodeOutlineColorSequence = {NamedColors[NamedColor::defaultOutline]};
            letterGroup.nodeLabelColorSequence = {NamedColors[NamedColor::defaultLabel]};
            letterGroup.groupOutlineColor = {NamedColors[NamedColor::defaultOutline], ColorPalette::undefinedColor};
            letterGroup.groupLabelColor = {NamedColors[NamedColor::defaultLabel], ColorPalette::undefinedColor};

            std::vector<std::string> letterGroupNames = {
                "lowercase",
                "lower case letters", "Lower case Latin letters",
                "upper case letters", "Upper case Latin letters",
                "numbers", "Numbers",
                "punctuation", "Punctuation",
                "ethiopic letters", "vowels etc", "vowel signs etc",
                "vowel-like letters", "character modifiers?",
                "hamza", "joiners",
                "arabic-indic numbers", "ascii punctuation",
            };
            for(auto& name : letterGroupNames)
                GroupColors[name] = letterGroup;
        }

        NamedColors[NamedColor::circleStarted] = safeColor(240);
        NamedColors[NamedColor::circleWaiting] = safeColor(241);
        NamedColors[NamedColor::circleStopped] = safeColor(242);

        std::array<ColorPalette::Color, 4> uiColors = {
            safeColor(10), safeColor(20), safeColor(30), safeColor(40)
        };

        KnownPalettes[paletteName] = new ColorPalette(
            HardcodedDefaultPalette, HardcodedDefaultPalette->PaletteName,
            NamedColors, GroupColors, uiColors, paletteName);
    }
    return true;
}

bool CColorIO::Parse(pugi::xml_document& document, const std::string, bool bUser)
{
	pugi::xml_node outer = document.document_element();

	if(strcmp(outer.name(),"colours") == 0) return ParseLegacy(document);

	if(strcmp(outer.name(),"colors") != 0 || outer.attribute("name").empty()) return false; // wrong type of root node or no name specified


	std::unordered_map<NamedColor::knownColorName, ColorPalette::Color> NamedColors;
	std::unordered_map<std::string, ColorPalette::GroupColorInfo> GroupColors;
	std::vector<ColorPalette::Color> UIPreviewColors;
	std::string parentName = outer.attribute("parentName").as_string(HardcodedDefaultPalette->PaletteName.c_str());
	std::string colorSchemeName = outer.attribute("name").as_string(); // definitely exists, we checked above

	for(pugi::xml_attribute attribute : outer.attributes())
	{
		if(strcmp(attribute.name(),"parentName") == 0 || strcmp(attribute.name(),"name") == 0) continue;

		if(strcmp(attribute.name(),"uiPreviewColors") == 0){
			UIPreviewColors = GetAttributeAsColorList(attribute);
			continue;
		}

		NamedColors[attribute.name()] = ColorPalette::Color(attribute.as_string());
	}

	for(pugi::xml_node groupInfo : outer.children())
	{
	    if(strcmp(groupInfo.name(),"groupColorInfo") != 0 || groupInfo.attribute("name").empty()) continue; //Ignore all others tags or groups without a name
		
		ColorPalette::GroupColorInfo group;
		group.groupColor = {GetAttributeAsColor(groupInfo.attribute("groupColor")), GetAttributeAsColor(groupInfo.attribute("altGroupColor"))};
		group.groupLabelColor = {GetAttributeAsColor(groupInfo.attribute("groupLabelColor")), GetAttributeAsColor(groupInfo.attribute("altGroupLabelColor"))};
		group.groupOutlineColor = {GetAttributeAsColor(groupInfo.attribute("groupOutlineColor")), GetAttributeAsColor(groupInfo.attribute("altGroupOutlineColor"))};

		group.nodeColorSequence = GetAttributeAsColorList(groupInfo.attribute("nodeColorSequence"));
		group.altNodeColorSequence = GetAttributeAsColorList(groupInfo.attribute("altNodeColorSequence"));
	    group.nodeOutlineColorSequence = GetAttributeAsColorList(groupInfo.attribute("nodeOutlineColorSequence"));
		group.altNodeOutlineColorSequence = GetAttributeAsColorList(groupInfo.attribute("altNodeOutlineColorSequence"));
		group.nodeLabelColorSequence = GetAttributeAsColorList(groupInfo.attribute("nodeLabelColorSequence"));
		group.altNodeLabelColorSequence = GetAttributeAsColorList(groupInfo.attribute("altNodeLabelColorSequence"));

		GroupColors[groupInfo.attribute("name").as_string()] = group;
	}

	//could not be loaded, determine default colors by search first group >=4 and sample four equally spaced colors
	if(UIPreviewColors.size() != 4){
		UIPreviewColors.clear();
		for(auto& [name, group] : GroupColors){
			if(group.nodeColorSequence.size() >= 4){
				UIPreviewColors.push_back(group.nodeColorSequence.front());
				UIPreviewColors.push_back(group.nodeColorSequence[group.nodeColorSequence.size() / 3]);
				UIPreviewColors.push_back(group.nodeColorSequence[group.nodeColorSequence.size() / 3 * 2]);
				UIPreviewColors.push_back(group.nodeColorSequence.back());
				break;
			}
		}
		// no fitting group found use default (error) colors
		if(UIPreviewColors.size() == 0){
			UIPreviewColors = {
			ColorPalette::Color(0, 0, 0, 255),
			ColorPalette::Color(255, 0, 255, 255),
			ColorPalette::Color(0, 0, 0, 255),
			ColorPalette::Color(255, 0, 255, 255)
			};
		}
	}
	std::array<ColorPalette::Color, 4> uiColorsArray;
	std::copy_n(std::make_move_iterator(UIPreviewColors.begin()), uiColorsArray.size(), uiColorsArray.begin());

	//"HardcodedDefault" is the parent for now, later on the parents get relinked by looking up the parentNames
	KnownPalettes[colorSchemeName] = new ColorPalette(HardcodedDefaultPalette, parentName, NamedColors, GroupColors, uiColorsArray, colorSchemeName);

	return true;
}

void CColorIO::RelinkParents()
{
	for(auto& [paletteName, palette] : KnownPalettes)
	{
	    palette->ParentPalette = FindPalette(palette->ParentPaletteName);
	}

	// try to detect and (temporarily) remove cycles in parenting, issuing a warning to users
	for(auto& [paletteName, palette] : KnownPalettes)
	{
	    std::vector<std::string> visited = {paletteName};

		const ColorPalette* current = palette;
		while(current->ParentPalette)
		{
			//already visited
			if(std::find(visited.begin(), visited.end(), current->ParentPalette->PaletteName) != visited.end())
			{
				std::string allVisited;
				for(std::string& s : visited) allVisited += s + "->";
				allVisited += current->ParentPalette->PaletteName;
				m_pMsgs->FormatMessage("Found cycle while parsing color-scheme parenting: %s", allVisited.c_str());
				KnownPalettes.erase(current->ParentPalette->PaletteName);
				RelinkParents(); //relink as now a palette was removed
				return;
			}
			visited.push_back(current->ParentPalette->PaletteName);
			current = current->ParentPalette;
		}
	}
}

void CColorIO::CreateDefault() {
	const std::unordered_map<NamedColor::knownColorName, ColorPalette::Color> NamedColors = {
        {NamedColor::background, ColorPalette::Color(255, 255, 255, 255)},
        {NamedColor::inputLine, ColorPalette::Color(255, 0, 0, 255)},
        {NamedColor::inputPosition, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::crosshair, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::rootNode, ColorPalette::Color(180, 238, 180, 255)},
        {NamedColor::defaultOutline, ColorPalette::Color(218, 218, 218, 255)},
        {NamedColor::defaultLabel, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::selectionHighlight, ColorPalette::Color(255, 0, 0, 255)},
        {NamedColor::circleOutline, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::circleStopped, ColorPalette::Color(255, 0, 0, 255)},
        {NamedColor::circleWaiting, ColorPalette::Color(240, 240, 0, 255)},
        {NamedColor::circleStarted, ColorPalette::Color(0, 255, 0, 255)},
        {NamedColor::firstStartBox, ColorPalette::Color(255, 0, 0, 255)},
        {NamedColor::secondStartBox, ColorPalette::Color(255, 255, 0, 255)},
        {NamedColor::twoPushDynamicActiveMarker, ColorPalette::Color(0, 255, 0, 255)},
        {NamedColor::twoPushDynamicInactiveMarker, ColorPalette::Color(0, 255, 255, 255)},
        {NamedColor::twoPushDynamicOuterGuides, ColorPalette::Color(180, 238, 180, 255)},
        {NamedColor::infoText, ColorPalette::Color(255, 255, 255, 255)},
        {NamedColor::infoTextBackground, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::warningText, ColorPalette::Color(255, 255, 0, 255)},
        {NamedColor::warningTextBackground, ColorPalette::Color(0, 0, 0, 255)},
        {NamedColor::gameGuide, ColorPalette::Color(255, 100, 204, 255)}
	};

	std::array<ColorPalette::Color, 4> uiColorsArray = {
	ColorPalette::Color(0, 0, 0, 255),
	ColorPalette::Color(255, 0, 255, 255),
	ColorPalette::Color(0, 0, 0, 255),
	ColorPalette::Color(255, 0, 255, 255)
	};

	HardcodedDefaultPalette = new ColorPalette(nullptr, "NonExistentRootRootPalette", NamedColors, {}, uiColorsArray, "HardcodedDefault"); //TODO: No groups for now, but will later be added
}
