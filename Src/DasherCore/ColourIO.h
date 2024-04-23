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
#include <vector>

namespace Dasher {
	class CColourIO;

	typedef struct ColorPalette {
		typedef struct Color
		{
			int Red = 0;
			int Green = 0;
			int Blue = 0;
			int Alpha = 1;
			Color(int Red, int Green, int Blue, int Alpha = 255) : Red(Red), Green(Green), Blue(Blue), Alpha(Alpha){};

			bool operator==(const Color& t) const
			{
			    return Red == t.Red && Green == t.Green && Blue == t.Blue && Alpha == t.Alpha;
			}
			bool operator!=(const Color& t) const
			{
			    return !operator==(t);
			}
			Color operator* (float x) const
			{
			    return {static_cast<int>(static_cast<float>(Red)*x),
			        static_cast<int>(static_cast<float>(Green) * x),
			        static_cast<int>(static_cast<float>(Blue) * x),
			        static_cast<int>(static_cast<float>(Alpha) * x)
			    };
			}
			Color operator+ (const Color& b) const
			{
			    return {Red + b.Red, Green + b.Green, Blue + b.Blue, Alpha + b.Alpha};
			}
			//ColorB * (1 - a) + ColorA * a
			static Color lerp(const Color& ColorA, const Color& ColorB, float a)
			{
			    return ColorB * (1.0f - a) + ColorA * a;
			}
		} Color;

		enum NamedColor
		{
			background = 0,
			mouseLine = 1,
			mouseCursor = 2,
			boxOutline = 3,
			text = 4,
			crosshair = 5,
			keyboardPosition = 6,
			rootNode = 7,
			controlNode = 8,
			space = 9
		};

		std::string ColorID;
		bool Mutable = false; // If from user we may play. If from system defaults this is immutable. User should take a copy.
		std::vector<Color> Colors; //Contains RGBA values
		inline static const Color noColor = {-1,-1,-1,0};
	} ColorPalette;
}

// Class for reading in color-scheme definitions, and storing all read schemes in a list.
class Dasher::CColourIO : public AbstractXMLParser {
public:
	///Construct a new ColourIO. It will have only a 'default' colour scheme;
	/// further schemes may be loaded in by calling the Parse... methods inherited
	/// from Abstract[XML]Parser.
	CColourIO(CMessageDisplay *pMsgs);
	virtual ~CColourIO() = default;
  
	void GetColours(std::vector<std::string> *ColourList) const;
	const ColorPalette & GetInfo(const std::string & ColourID);

	bool Parse(pugi::xml_document& document, const std::string filePath, bool bUser) override;

private:
	std::map < std::string, ColorPalette > KnownPaletts; // map short names (file names) to descriptions

	void CreateDefault();         // Give the user a default colour scheme rather than nothing if anything goes horribly wrong.
};
