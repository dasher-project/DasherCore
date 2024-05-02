#include "ColorPalette.h"

#include <regex>
#include <string>

using namespace Dasher;

ColorPalette::Color::Color(int Red, int Green, int Blue, int Alpha): Red(Red), Green(Green), Blue(Blue), Alpha(Alpha){}


ColorPalette::Color::Color(const std::string& HexString)
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
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor && search->second.groupColor.second != undefinedColor) ? search->second.groupColor.second : search->second.groupColor.first;
        if(result != undefinedColor) return result;
    }
    return (ParentPalette) ? ParentPalette->GetGroupColor(GroupName, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetGroupOutlineColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor && search->second.groupOutlineColor.second != undefinedColor) ? search->second.groupOutlineColor.second : search->second.groupOutlineColor.first;
        if(result != undefinedColor) return result;
    }
    return (ParentPalette) ? ParentPalette->GetGroupOutlineColor(GroupName, UseAltColor) : GetNamedColor(NamedColor::defaultOutline);
}

const ColorPalette::Color& ColorPalette::GetGroupLabelColor(const std::string& GroupName, const bool& UseAltColor) const
{
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const Color& result = (UseAltColor && search->second.groupLabelColor.second != undefinedColor) ? search->second.groupLabelColor.second : search->second.groupLabelColor.first;
        if(result != undefinedColor) return result;
    }
    return (ParentPalette) ? ParentPalette->GetGroupLabelColor(GroupName, UseAltColor) : GetNamedColor(NamedColor::defaultLabel);
}

const ColorPalette::Color& ColorPalette::GetNodeColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;

        if(UseAltColor && !Group.altNodeColorSequence.empty())
        {
            return Group.altNodeColorSequence[nodeIndexInGroup % Group.altNodeColorSequence.size()];
        }

        if(Group.nodeColorSequence.empty() && ParentPalette) return ParentPalette->GetNodeColor(GroupName, nodeIndexInGroup, UseAltColor);
        return Group.nodeColorSequence[nodeIndexInGroup % Group.nodeColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeColor(GroupName, nodeIndexInGroup, UseAltColor) : noColor;
}

const ColorPalette::Color& ColorPalette::GetNodeOutlineColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;

        if(UseAltColor && !Group.altNodeOutlineColorSequence.empty())
        {
            return Group.altNodeOutlineColorSequence[nodeIndexInGroup % Group.altNodeOutlineColorSequence.size()];
        }

        if(Group.nodeOutlineColorSequence.empty() && ParentPalette) return ParentPalette->GetNodeOutlineColor(GroupName, nodeIndexInGroup, UseAltColor);
        return Group.nodeOutlineColorSequence[nodeIndexInGroup % Group.nodeOutlineColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeOutlineColor(GroupName, nodeIndexInGroup, UseAltColor) : GetNamedColor(NamedColor::defaultOutline);
}

const ColorPalette::Color& ColorPalette::GetNodeLabelColor(const std::string& GroupName, const int& nodeIndexInGroup,
    const bool& UseAltColor) const
{
    if(GroupName.empty()) return noColor;

    if (const auto& search = GroupColors.find(GroupName); search != GroupColors.end())
    {
        const GroupColorInfo& Group = search->second;

        if(UseAltColor && !Group.altNodeLabelColorSequence.empty())
        {
            return Group.altNodeLabelColorSequence[nodeIndexInGroup % Group.altNodeLabelColorSequence.size()];
        }

        if(Group.nodeLabelColorSequence.empty() && ParentPalette) return ParentPalette->GetNodeLabelColor(GroupName, nodeIndexInGroup, UseAltColor);
        return Group.nodeLabelColorSequence[nodeIndexInGroup % Group.nodeLabelColorSequence.size()];
    }
    return (ParentPalette) ? ParentPalette->GetNodeLabelColor(GroupName, nodeIndexInGroup, UseAltColor) : GetNamedColor(NamedColor::defaultLabel);
}
