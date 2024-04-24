#pragma once

#include "AbstractXMLParser.h"

#include <map>
#include <vector>

#include "ColorPalette.h"

namespace Dasher {
    // Class for reading in color-scheme definitions, and storing all read schemes in a list.
    class CColorIO : public AbstractXMLParser {
    public:
	    ///Construct a new ColourIO. It will have only a 'default' colour scheme;
	    /// further schemes may be loaded in by calling the Parse... methods inherited
	    /// from Abstract[XML]Parser.
	    CColorIO(CMessageDisplay *pMsgs);
        ~CColorIO() override;
        void GetKnownPalettes(std::vector<std::string>* ColourList) const;

        const ColorPalette* FindPalette(const std::string& ColorPaletteName);

        bool Parse(pugi::xml_document& document, const std::string filePath, bool bUser) override;

        //Sets all Parent Pointers for the Color Palettes if the parent is known. Else we set them to "Default".
        void RelinkParents();

    private:
        static ColorPalette::Color GetAttributeAsColor(pugi::xml_attribute attribute, ColorPalette::Color defaultColor);
        static std::vector<ColorPalette::Color> GetAttributeAsColorList(pugi::xml_attribute attribute, ColorPalette::Color defaultColor);

	    std::map <std::string, ColorPalette*> KnownPalettes;

	    void CreateDefault();         // Give the user a default colour scheme rather than nothing if anything goes horribly wrong.
    };
}
