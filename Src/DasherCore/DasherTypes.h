// DasherTypes.h
//
// Copyright (c) 2001-2002 David Ward

#pragma once

// We use our own version of hungarian notation to indicate
// the type of variables:
//
//    i       - integer and enumerated types 
//    c       - char
//    str     - STL string
//    sz      - char* string
//    b       - boolean
//    p       - pointer (to a primative type or to an object)
//    pp      - pointer to a pointer (and so on)
//    v       - STL vector
//    map     - STL map
//    d       - float or double
//    s       - structure 
//    o       - object
//    h       - HANDLE type in Windows
//
// Class member variables and global variables should 
// have the additional prefixes:
//
//    m_      - member variables 
//    g_      - global variablse
//    s_      - static member variables
//
// Variables names (local and member) should capitalize each 
// new word and don't use underscores (except as above).
//

#include <string>
#include <vector>
#include <cstdint>

//Some typedefs to not having to change this in all of the DasherCore
namespace Dasher
{
  // DasherModel co-ordinates are of type myint
  typedef int64_t myint;
  typedef int64_t dasherint;

  // All screen co-ordinates are of type screenint
  typedef int32_t screenint;

  //! Structure defining a point on the screen 
	typedef struct point
	{
		screenint x = 0;
		screenint y = 0;
    point() = default;
    point(screenint X, screenint Y) : x(X), y(Y) {}
	} point;

  // Using a signed symbol type allows "Out of band" ie negative {{{
  // values to be used to flag non-symbol data. For example commands
  // in dasher nodes.
  //typedef unsigned int symbol; // }}}
  typedef int symbol;

	// Used for Cube Rendering only
	struct CubeDepthLevel
	{
	    Dasher::myint extrusionLevel;
	    Dasher::myint groupRecursionDepth;
	};

namespace Options
{
    // Numbers should be applied to elements of the following two enumerations as these preferences may be stored to file. Constancy between
    // versions is a good idea. It should *not* be assumed that the numbers map onto anything useful. Different codepages may be appropriate on different systems for different character sets.
    enum FileEncodingFormats { UserDefault = -1, UTF8 = 65001, UTF16LE = 1200, UTF16BE = 1201 }; 

    enum AlphabetTypes { MyNone = 0, Arabic = 1256, Baltic = 1257, CentralEurope = 1250, ChineseSimplified = 936, ChineseTraditional = 950, Cyrillic = 1251, Greek = 1253, Hebrew = 1255, Japanese = 932, Korean = 949, Thai = 874, Turkish = 1254, VietNam = 1258, Western = 1252 };

    enum ScreenOrientations { AlphabetDefault = -2, LeftToRight = 0, RightToLeft = 1, TopToBottom = 2, BottomToTop = 3 };

    enum FontSize { Normal = 1, Big = 2, VBig = 4 };

	enum RenderingShapeTypes { DISJOINT_RECTANGLE = 0, OVERLAPPING_RECTANGLE = 1, TRIANGLE = 2, TRUNCATED_TRIANGLE = 3, QUADRIC = 4, CIRCLE = 5, CUBE = 6 };

    enum ScreenGeometry {old_style = 0, square_no_xhair = 1, squish = 2, squish_and_log = 3};
  }

namespace Keys
{
	// Virtual Keys that are used in the KeyDown/KeyUp Events. Suggestions/Intentions are marked and numbers are assigned based on old "int" based keys
	enum VirtualKey
	{
		Big_Start_Stop_Key = 0, //Typically Space
		Button_1 = 1,
		Button_2 = 2,
		Button_3 = 3,
		Button_4 = 4,
		//Reserve some buttons for the DasherButtons/Menu Input filter as theoretically it can have as many as wanted

		Primary_Input = 100, //Typically Mouse Left
		Secondary_Input = 101, //Typically Mouse Right
		Tertiary_Input = 102, //Typically Third Mouse Button
		Invalid_Key = -1
	};
}

  // Types added so model can report back what it has done for
  // user logging purposes.
  struct SymbolProb
  {
  public:
    SymbolProb(symbol _sym, const std::string &sDisp, double _prob)
		: sym(_sym), strDisplay(sDisp), prob(_prob)
	{
	}

    symbol          sym;
    std::string     strDisplay; //easiest to generate at source!
    double          prob;
  };

  typedef std::vector<SymbolProb>         VECTOR_SYMBOL_PROB;
}

