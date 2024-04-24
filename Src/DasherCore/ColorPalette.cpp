#include "ColorPalette.h"

#include <regex>
#include <string>

using namespace Dasher;

ColorPalette::Color::Color(unsigned int Red, unsigned int Green, unsigned int Blue, unsigned int Alpha): Red(Red), Green(Green), Blue(Blue), Alpha(Alpha){}


ColorPalette::Color::Color(const std::string& HexString)
{
    static const std::regex pattern("#?([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})?"); // only instantiate once, thus static

    std::smatch match;
    if (std::regex_match(HexString, match, pattern) && (match.size() == 3 || match.size() == 4))
    {
        Red = std::stoul(match[1].str(), nullptr, 16);
        Green = std::stoul(match[2].str(), nullptr, 16);
        Blue = std::stoul(match[3].str(), nullptr, 16);
        Alpha = (match.size() == 4) ? std::stoul(match[4].str(), nullptr, 16) : 255;
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
    return {static_cast<unsigned int>(static_cast<float>(Red) * x),
        static_cast<unsigned int>(static_cast<float>(Green) * x),
        static_cast<unsigned int>(static_cast<float>(Blue) * x),
        static_cast<unsigned int>(static_cast<float>(Alpha) * x)
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
    const std::unordered_map<std::string, GroupColorInfo>& GroupColors) : ParentPalette(ParentPalette), ParentPaletteName(
                                                                              std::move(ParentPaletteName)), NamedColors(NamedColors), GroupColors(GroupColors)
{
}

const ColorPalette::Color& ColorPalette::GetNamedColor(const NamedColor::knownColorName& NamedColor) const
{
    if (const auto& search = NamedColors.find(NamedColor); search != NamedColors.end())
    {
        return search->second;
    }
    return (ParentPalette) ? ParentPalette->GetNamedColor(NamedColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetGroupColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor) ? search->second.groupColor.first : search->second.groupColor.second;
        return (result == noColor && ParentPalette) ? ParentPalette->GetGroupColor(GroupName, UseAltColor) : noColor;
    }
    return (ParentPalette) ? ParentPalette->GetGroupColor(GroupName, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetGroupOutlineColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor) ? search->second.groupOutlineColor.first : search->second.groupOutlineColor.second;
        return (result == noColor && ParentPalette) ? ParentPalette->GetGroupColor(GroupName, UseAltColor) : noColor;
    }
    return (ParentPalette) ? ParentPalette->GetGroupOutlineColor(GroupName, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetGroupLabelColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor) ? search->second.groupOutlineColor.first : search->second.groupOutlineColor.second;
        return (result == noColor && ParentPalette) ? ParentPalette->GetGroupColor(GroupName, UseAltColor) : noColor;
    }
    return (ParentPalette) ? ParentPalette->GetGroupLabelColor(GroupName, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetNodeColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;

        if(UseAltColor)
        {
            return Group.altNodeColorSequence[nodeIndexInGroup % Group.altNodeColorSequence.size()];
        }

        return Group.nodeColorSequence[nodeIndexInGroup % Group.nodeColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeColor(GroupName, nodeIndexInGroup, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetNodeOutlineColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;
        if(UseAltColor)
        {
            return Group.altNodeOutlineColorSequence[nodeIndexInGroup % Group.altNodeOutlineColorSequence.size()];
        }

        return Group.nodeOutlineColorSequence[nodeIndexInGroup % Group.nodeOutlineColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeOutlineColor(GroupName, nodeIndexInGroup, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetNodeLabelColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;
        if(UseAltColor)
        {
            return Group.altNodeLabelColorSequence[nodeIndexInGroup % Group.altNodeLabelColorSequence.size()];
        }

        return Group.nodeLabelColorSequence[nodeIndexInGroup % Group.nodeLabelColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeLabelColor(GroupName, nodeIndexInGroup, UseAltColor) : noColor;
}
