// ColourIO.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <string>

namespace Dasher {
namespace NamedColor {
typedef std::string knownColorName;
static const knownColorName background = "backgroundColor";
static const knownColorName inputLine = "inputLineColor";
static const knownColorName inputPosition = "inputPositionColor";
static const knownColorName crosshair = "crosshairColor";
static const knownColorName rootNode = "rootNodeColor";
static const knownColorName conversionNode = "conversionNodeColor";
static const knownColorName defaultOutline = "defaultOutlineColor";
static const knownColorName defaultLabel = "defaultLabelColor";
static const knownColorName selectionHighlight = "selectionHighlightColor";
static const knownColorName selectionInactive = "selectionInactiveColor";
static const knownColorName circleOutline = "circleOutlineColor";
static const knownColorName circleStopped = "circleStoppedColor";
static const knownColorName circleWaiting = "circleWaitingColor";
static const knownColorName circleStarted = "circleStartedColor";
static const knownColorName firstStartBox = "firstStartBoxColor";
static const knownColorName secondStartBox = "secondStartBoxColor";
static const knownColorName twoPushDynamicActiveMarker = "twoPushDynamicActiveMarkerColor";
static const knownColorName twoPushDynamicInactiveMarker = "twoPushDynamicInactiveMarkerColor";
static const knownColorName twoPushDynamicOuterGuides = "twoPushDynamicOuterGuidesColor";
static const knownColorName infoText = "infoTextColor";
static const knownColorName infoTextBackground = "infoTextBackgroundColor";
static const knownColorName warningText = "warningTextColor";
static const knownColorName warningTextBackground = "warningTextBackgroundColor";
static const knownColorName gameGuide = "gameGuideColor";
}; // namespace NamedColor

class ColorPalette {
  public:
    // Visual classification of a palette for system-appearance matching (RFC 0007).
    // Unspecified palettes (e.g. legacy-format files without metadata) are still
    // pairable via a bidirectional companion lookup.
    enum class Appearance : int { Unspecified = 0, Light = 1, Dark = 2 };

    typedef struct Color {
        int Red = 0;
        int Green = 0;
        int Blue = 0;
        int Alpha = 255;

        Color() {};
        Color(int Red, int Green, int Blue, int Alpha = 255);
        Color(const std::string& HexString, const Color& defaultColor = undefinedColor);

        bool operator==(const Color& t) const;
        bool operator!=(const Color& t) const;
        Color operator*(float x) const;
        Color operator+(const Color& b) const;
        bool isFullyTransparent() const { return Alpha == 0; }
        bool isFullyOpaque() const { return Alpha == 255; }
        Color toGrayScale() const;

        // self * (1 - a) + ColorB * a
        Color lerp(const Color& ColorB, float a) const;

        // ColorB * (1 - a) + ColorA * a
        static Color lerp(const Color& ColorA, const Color& ColorB, float a);
    } Color;

    inline static const Color noColor = {0, 0, 0, 0};
    inline static const Color undefinedColor = {-1, -1, -1, -1};
    inline static const Color black = {0, 0, 0, 1};
    inline static const Color white = {1, 1, 1, 1};

    // Represents the colors for one named group
    // each std::pair<Color,Color> represents a color and an alternative color for the same purpose to not nest the same
    // colors each std::vector<std::pair<Color,Color>> is iterated for the nodes (letters) in each group, to assign a
    // color to each of them
    typedef struct GroupColorInfo {
        GroupColorInfo() {}

        std::vector<Color> nodeColorSequence;
        std::vector<Color> nodeLabelColorSequence;
        std::vector<Color> nodeOutlineColorSequence;
        std::vector<Color> altNodeColorSequence;
        std::vector<Color> altNodeLabelColorSequence;
        std::vector<Color> altNodeOutlineColorSequence;

        std::pair<Color, Color> groupColor = {undefinedColor, undefinedColor};
        std::pair<Color, Color> groupOutlineColor = {undefinedColor, undefinedColor};
        std::pair<Color, Color> groupLabelColor = {undefinedColor, undefinedColor};
    } GroupColorInfo;

    ColorPalette(ColorPalette* ParentPalette, std::string ParentPaletteName,
                 const std::unordered_map<NamedColor::knownColorName, Color>& NamedColors,
                 const std::unordered_map<std::string, GroupColorInfo>& GroupColors,
                 std::array<Color, 4> UIPreviewColors, std::string PaletteName,
                 Appearance appearance = Appearance::Unspecified, std::string companionName = "");
    const Color& GetAltColor(const std::vector<Color>& NormalColors, const std::vector<Color>& AltColors, bool useAlt,
                             int Index) const;
    const Color& GetAltColor(const Color& NormalColor, const Color& AltColor, bool useAlt) const;

    // We need both links to the parentPalette, as we first only parse and link the palettes afterwards
    const ColorPalette* ParentPalette = nullptr;
    std::string ParentPaletteName;
    std::string PaletteName;

    Appearance AppearanceValue = Appearance::Unspecified; // RFC 0007: light/dark classification
    std::string CompanionName;                            // RFC 0007: opposite-appearance partner name

    const Color& GetNamedColor(const NamedColor::knownColorName& NamedColor, bool AskParent = true) const;

    const std::array<Color, 4>& GetUIPreviewColors() const;

    const Color& GetGroupColor(const std::string& GroupName, const bool& UseAltColor) const;
    const Color& GetGroupOutlineColor(const std::string& GroupName, const bool& UseAltColor,
                                      bool UseDefaultColor = true) const;
    const Color& GetGroupLabelColor(const std::string& GroupName, const bool& UseAltColor,
                                    bool UseDefaultColor = true) const;

    const Color& GetNodeColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor) const;
    const Color& GetNodeOutlineColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor,
                                     bool UseDefaultColor = true) const;
    const Color& GetNodeLabelColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor,
                                   bool UseDefaultColor = true) const;

  private:
    std::unordered_map<NamedColor::knownColorName, Color> NamedColors;
    std::unordered_map<std::string, GroupColorInfo> GroupColors;
    std::array<Color, 4> UIPreviewColors;
};

} // namespace Dasher
