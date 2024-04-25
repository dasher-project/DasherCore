// ColourIO.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 Iain Murray
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "AbstractXMLParser.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace Dasher {
	namespace NamedColor
	{
		typedef std::string knownColorName;
		static const knownColorName background = "backgroundColor";
		static const knownColorName inputLine = "inputLineColor";
		static const knownColorName inputPosition = "inputPositionColor";
		static const knownColorName crosshair = "crosshairColor";
		static const knownColorName rootNode = "rootNodeColor";
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
	};

	class ColorPalette {
	public:
		typedef struct Color
		{
			unsigned int Red = 0;
			unsigned int Green = 0;
			unsigned int Blue = 0;
			unsigned int Alpha = 255;

			Color(unsigned int Red, unsigned int Green, unsigned int Blue, unsigned int Alpha = 255);
            Color(const std::string& HexString);

			bool operator==(const Color& t) const;
            bool operator!=(const Color& t) const;
            Color operator* (float x) const;
            Color operator+ (const Color& b) const;

            //ColorB * (1 - a) + ColorA * a
			static Color lerp(const Color& ColorA, const Color& ColorB, float a);
        } Color;

		inline static const Color noColor = {0,0,0,0};

		// Represents the colors for one named group
		// each std::pair<Color,Color> represents a color and an alternative color for the same purpose to not nest the same colors
		// each std::vector<std::pair<Color,Color>> is iterated for the nodes (letters) in each group, to assign a color to each of them
		typedef struct GroupColorInfo
		{
            GroupColorInfo(){}

            std::vector<Color> nodeColorSequence;
			std::vector<Color> nodeLabelColorSequence;
			std::vector<Color> nodeOutlineColorSequence;
			std::vector<Color> altNodeColorSequence;
			std::vector<Color> altNodeLabelColorSequence;
			std::vector<Color> altNodeOutlineColorSequence;

			std::pair<Color,Color> groupColor = {noColor, noColor};
			std::pair<Color,Color> groupOutlineColor = {noColor, noColor};
			std::pair<Color,Color> groupLabelColor = {noColor, noColor};
		} GroupColorInfo;

		ColorPalette(ColorPalette* ParentPalette, std::string ParentPaletteName, const std::unordered_map<NamedColor::knownColorName, Color>& NamedColors, const std::unordered_map<std::string, GroupColorInfo>& GroupColors);

		// We need both links to the parentPalette, as we first only parse and link the palettes afterwards
		const ColorPalette* ParentPalette = nullptr;
		std::string ParentPaletteName;

		const Color& GetNamedColor(const NamedColor::knownColorName& NamedColor) const;

		const Color& GetGroupColor(const std::string& GroupName, const bool& UseAltColor) const;
		const Color& GetGroupOutlineColor(const std::string& GroupName, const bool& UseAltColor) const;
		const Color& GetGroupLabelColor(const std::string& GroupName, const bool& UseAltColor) const;

		const Color& GetNodeColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor) const;
		const Color& GetNodeOutlineColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor) const;
		const Color& GetNodeLabelColor(const std::string& GroupName, const int& nodeIndexInGroup, const bool& UseAltColor) const;

    private:
	    std::unordered_map<NamedColor::knownColorName, Color> NamedColors;
		std::unordered_map<std::string, GroupColorInfo> GroupColors;
	};

   
}
