#include "ColorPalette.h"

#include <regex>
#include <string>

using namespace Dasher;

ColorPalette::Color::Color(int Red, int Green, int Blue, int Alpha): Red(Red), Green(Green), Blue(Blue), Alpha(Alpha){}


ColorPalette::Color::Color(const std::string& HexString, const Color& defaultColor)
{
    static const std::regex pattern("#?([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})?"); // only instantiate once, thus static

    std::smatch match;
    if (std::regex_match(HexString, match, pattern) && (match.size() == 5 || match.size() == 6))
    {
        Red = static_cast<int>(std::stoul(match[1].str(), nullptr, 16));
        Green = static_cast<int>(std::stoul(match[2].str(), nullptr, 16));
        Blue = static_cast<int>(std::stoul(match[3].str(), nullptr, 16));
        Alpha = (match[4].matched) ? static_cast<int>(std::stoul(match[4].str(), nullptr, 16)) : 255;
    }
    else // Either match or copy default
    {
        Red = defaultColor.Red;
        Green = defaultColor.Green;
        Blue = defaultColor.Blue;
        Alpha = defaultColor.Alpha;
    }
}

bool ColorPalette::Color::operator==(const Color& t) const
{
    return Red == t.Red && Green == t.Green && Blue == t.Blue && Alpha == t.Alpha;
}

bool ColorPalette::Color::operator!=(const Color& t) const
{
    return !operator==(t);
}

ColorPalette::Color ColorPalette::Color::operator*(float x) const
{
    return {static_cast<int>(static_cast<float>(Red) * x),
        static_cast<int>(static_cast<float>(Green) * x),
        static_cast<int>(static_cast<float>(Blue) * x),
        static_cast<int>(static_cast<float>(Alpha) * x)
    };
}

ColorPalette::Color ColorPalette::Color::operator+(const Color& b) const
{
    return {Red + b.Red, Green + b.Green, Blue + b.Blue, Alpha + b.Alpha};
}

ColorPalette::Color ColorPalette::Color::lerp(const Color& ColorA, const Color& ColorB, float a)
{
    return ColorB * (1.0f - a) + ColorA * a;
}

ColorPalette::ColorPalette(ColorPalette* ParentPalette, std::string ParentPaletteName,
                           const std::unordered_map<NamedColor::knownColorName, Color>& NamedColors,
    const std::unordered_map<std::string, GroupColorInfo>& GroupColors, std::string PaletteName) : ParentPalette(ParentPalette), ParentPaletteName(
                                                                              std::move(ParentPaletteName)), NamedColors(NamedColors), GroupColors(GroupColors), PaletteName(PaletteName)
{
}

const ColorPalette::Color& ColorPalette::GetAltColor(const std::vector<Color>& NormalColors, const std::vector<Color>& AltColors, bool useAlt, int Index) const
{
    if(useAlt && !AltColors.empty()) return AltColors[Index % AltColors.size()];
    if(!NormalColors.empty()) return NormalColors[Index % NormalColors.size()];
    return undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetAltColor(const Color& NormalColor, const Color& AltColor, bool useAlt) const
{
    if(useAlt && AltColor != undefinedColor) return AltColor;
    if(NormalColor != undefinedColor) return NormalColor;
    return undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetNamedColor(const NamedColor::knownColorName& NamedColor, bool AskParent) const
{
    if (const auto& search = NamedColors.find(NamedColor); search != NamedColors.end())
    {
        return search->second;
    }
    return (ParentPalette && AskParent) ? ParentPalette->GetNamedColor(NamedColor) : undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetGroupColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = GetAltColor(search->second.groupColor.first, search->second.groupColor.second, UseAltColor);
        if(result != undefinedColor) return result;
    }
    if(ParentPalette) return ParentPalette->GetGroupColor(GroupName, UseAltColor);
    return noColor;
}

const ColorPalette::Color& ColorPalette::GetGroupOutlineColor(const std::string& GroupName, const bool& UseAltColor, bool UseDefaultColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = GetAltColor(search->second.groupOutlineColor.first, search->second.groupOutlineColor.second, UseAltColor);
        if(result != undefinedColor) return result;
    }
    
    if(ParentPalette) {
        const Color& color = ParentPalette->GetGroupOutlineColor(GroupName, UseAltColor, false);
        if(color != undefinedColor) return color;
    }

    if(UseDefaultColor)
    {
        return GetNamedColor(NamedColor::defaultOutline, true);
    }

    return undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetGroupLabelColor(const std::string& GroupName, const bool& UseAltColor, bool UseDefaultColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = GetAltColor(search->second.groupLabelColor.first, search->second.groupLabelColor.second, UseAltColor);
        if(result != undefinedColor) return result;
    }
    
    if(ParentPalette) {
        const Color& color = ParentPalette->GetGroupLabelColor(GroupName, UseAltColor, false);
        if(color != undefinedColor) return color;
    }

    if(UseDefaultColor)
    {
        return GetNamedColor(NamedColor::defaultLabel, true);
    }

    return undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetNodeColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& color = GetAltColor(search->second.nodeColorSequence, search->second.altNodeColorSequence, UseAltColor, nodeIndexInGroup);
        if(color != undefinedColor) return color;
    }
    if(ParentPalette) return ParentPalette->GetNodeColor(GroupName, nodeIndexInGroup, UseAltColor);

    return noColor;
}

const ColorPalette::Color& ColorPalette::GetNodeOutlineColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor, bool UseDefaultColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = GetAltColor(search->second.nodeOutlineColorSequence, search->second.altNodeOutlineColorSequence, UseAltColor, nodeIndexInGroup);
        if(result != undefinedColor) return result;
    }
    
    if(ParentPalette) {
        const Color& color = ParentPalette->GetNodeOutlineColor(GroupName, UseAltColor, false);
        if(color != undefinedColor) return color;
    }

    if(UseDefaultColor)
    {
        return GetNamedColor(NamedColor::defaultOutline, true);
    }

    return undefinedColor;
}

const ColorPalette::Color& ColorPalette::GetNodeLabelColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor, bool UseDefaultColor) const
{
    if(GroupName.empty()) return undefinedColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = GetAltColor(search->second.nodeLabelColorSequence, search->second.altNodeLabelColorSequence, UseAltColor, nodeIndexInGroup);
        if(result != undefinedColor) return result;
    }
    
    if(ParentPalette) {
        const Color& color = ParentPalette->GetNodeLabelColor(GroupName, UseAltColor, false);
        if(color != undefinedColor) return color;
    }

    if(UseDefaultColor)
    {
        return GetNamedColor(NamedColor::defaultLabel, true);
    }

    return undefinedColor;
}
