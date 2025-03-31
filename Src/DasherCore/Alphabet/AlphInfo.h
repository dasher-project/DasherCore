// AlphInfo.h
//
// Copyright (c) 2007 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef __ALPHINFO_H__
#define __ALPHINFO_H__


#include "../DasherTypes.h"
#include "GroupInfo.h"

#include <string>
#include <vector>

#include "Actions.h"

namespace Dasher {
  class CAlphInfo;
  class CAlphIO;
}

/// \ingroup Alphabet
/// @{

/// This structure completely describes the characters used in alphabet.
/// It maps from the "symbol" type (integers, starting at 1 for alphabet
/// characters, with 0 used for error / "unknown") to display text (that
/// rendered onto the canvas - e.g. a box or "_" for a space character),
/// text (that which is written/output, or indeed, read in by CAlphabetMap),
/// colour (an index into the current colour scheme; note values below 130
/// are increased by 130 on alternate offsets, this is known as the "phase"),
/// also foreground colour information (but these does not seem to be used
/// ATM).
///
/// One CAlphInfo object is created per alphabet when the alphabet.*.xml
/// files are read in by CAlphIO, and a CAlphabetMap object is created for
/// the alphabet currently in use (and deleted when the alphabet is changed).
///
/// Note the group structure stored by inheriting from SGroupInfo; these are filled
/// with iStart==1 (as symbol numbers are 1-indexed; 0 is reserved to indicate an
/// "unknown symbol", and for element 0 of the prob. array to contain a 0, and
/// symbol -1 indicates End-Of-Stream), and iEnd == one more than the number of
/// "text" symbols (i.e. inc space and para, but no control/conversion start/end)
/// - this is for consistency with SGroupInfo, preserving that iEnd is one more
/// than the highest valid index.
class Dasher::CAlphInfo : public SGroupInfo {
public:
  ///Format a character ready to write to a training file, by doubling
  /// up any escape character (context-switch / conversion-start)
  std::string escape(const std::string &ch) const;
  
  const std::string &GetID() const {return AlphID;}

  Options::ScreenOrientations GetOrientation() const {return Orientation;}

  const std::string & GetTrainingFile() const {return TrainingFile;}

  const std::string & GetPalette() const {return PreferredColors;}

  //Determine that this character denotes a word gap
  bool SymbolIsSpaceCharacter(symbol s) const {return std::isspace(m_vCharacters[s-1].Text[0]);}
  bool SymbolPrintsNewLineCharacter(symbol s) const {return m_vCharacters[s-1].Text == "\n";}

  const std::vector<Action*>& GetCharDoActions(symbol s) const {return m_vCharacterDoActions[s-1];}
  const std::vector<Action*>& GetCharUndoActions(symbol s) const {return m_vCharacterUndoActions[s-1];}

  //symbol GetStartConversionSymbol() const;
  //symbol GetEndConversionSymbol() const;

  /// return display string for i'th symbol
  const std::string & GetDisplayText(symbol i) const {return m_vCharacters[i-1].Display;}

  /// return text for edit box for i'th symbol
  const std::string & GetText(symbol i) const {return m_vCharacters[i-1].Text;}
  const double GetSymbolFixedProbability(symbol i) const {return m_vCharacters[i-1].fixedProbability;}
  const double GetSymbolSpeedMultiplier(symbol i) const {return m_vCharacters[i-1].speedFactor;}

  int getColorGroupOffset(symbol i) const
  {
      return m_vCharacters[i-1].ColorGroupOffset;
  };

  std::string& getColorGroup(symbol i) const
  {
      return m_vCharacters[i-1].parentGroup->colorGroup;
  };

  const std::string &GetDefaultContext() const {return m_strDefaultContext;}

  ///A single unicode character to use as an escape sequence in training files
  ///to indicate context-switching commands; 0-length => don't use context-switching commands.
  /// Defaults to § if not specified in alphabet.
  const std::string &GetContextEscapeChar() const {return m_strCtxChar;}

  ///0 = normal alphabet, contains symbols to output
  ///1 = Japanese (defunct)
  ///2 = Mandarin: symbols are merely phonemes, and match up (via displaytext)
  /// with groups in a second alphabet, identified by strConversionTarget,
  /// which contains actual output symbols possibly including duplicates;
  /// all this handled by MandarinAlphMgr (+MandarinTrainer, PPMPYLanguageModel).
  enum alphabetConversion
  {
      None = 0,
      Mandarin = 2,
      RoutingContextInsensitive = 3,
      RoutingContextSensitive = 4
  };
  alphabetConversion m_iConversionID;

  ///Single-unicode characters used in the training file to delimit the name of a group
  /// containing the next symbol, in order to disambiguate which group (=route, pronunciation)
  /// was used to produce the symbol in this case (see MandarinTrainer).
  /// Only used if m_iConversionID==2, 3 or 4. Default to "<" and ">"
  std::string m_strConversionTrainStart = "<";
  std::string m_strConversionTrainStop = ">";

  ~CAlphInfo();

private:
  friend class CAlphIO;
  CAlphInfo();
  // Basic information
  std::string AlphID;

  // Complete description of the alphabet:
  std::string TrainingFile;
  std::string PreferredColors;
  Options::ScreenOrientations Orientation;

  std::string m_strDefaultContext;
  std::string m_strCtxChar;

protected:
  struct character {
    character() = default;

    std::string Display;
    std::string Text;
    SGroupInfo* parentGroup = nullptr;
    int ColorGroupOffset = -1; //Offset within group
    float fixedProbability = -1; //fixed probability, resulting in fixed size later on. Currently not supported but parsed already.
    float speedFactor = -1; //allows for slowdown in this box
  };
  std::vector<character> m_vCharacters;
  std::vector<std::vector<Action*>> m_vCharacterDoActions = {};
  std::vector<std::vector<Action*>> m_vCharacterUndoActions = {};

  void copyCharacterFrom(const CAlphInfo *other, int idx);
};


/// @}

#endif /* #ifndef __AlphInfo_h__ */
