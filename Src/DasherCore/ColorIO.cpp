#include "ColorIO.h"
#include <algorithm>
#include <string>

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

bool CColorIO::Parse(pugi::xml_document& document, const std::string, bool bUser)
{
	pugi::xml_node outer = document.document_element();

	if(strcmp(outer.name(),"colors") != 0 || outer.attribute("name").empty()) return false; // wrong type of root node or no name specified


	std::unordered_map<NamedColor::knownColorName, ColorPalette::Color> NamedColors;
	std::unordered_map<std::string, ColorPalette::GroupColorInfo> GroupColors;
	std::string parentName = outer.attribute("parentName").as_string(HardcodedDefaultPalette->PaletteName.c_str());
	std::string colorSchemeName = outer.attribute("name").as_string(); // definitely exists, we checked above

	for(pugi::xml_attribute attribute : outer.attributes())
	{
		if(strcmp(attribute.name(),"parentName") == 0 || strcmp(attribute.name(),"name") == 0) continue;

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

	//"HardcodedDefault" is the parent for now, later on the parents get relinked by looking up the parentNames
	KnownPalettes[colorSchemeName] = new ColorPalette(HardcodedDefaultPalette, parentName, NamedColors, GroupColors, colorSchemeName);

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
				m_pMsgs->FormatMessageWithString("Found cycle while parsing color-scheme parenting: %s", allVisited.c_str());
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

	HardcodedDefaultPalette = new ColorPalette(nullptr, "NonExistentRootRootPalette", NamedColors, {}, "HardcodedDefault"); //TODO: No groups for now, but will later be added
}
