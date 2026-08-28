// AlphIO.h
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

#pragma once

#include "DasherCore/AbstractXMLParser.h"

#include "DasherCore/DasherTypes.h"
#include "DasherCore/Alphabet/AlphInfo.h"

#include <map>
#include <pugixml.hpp>
#include <vector>

namespace Dasher {
class CAlphIO;
}

/// \ingroup Alphabet
/// @{

/// This class is used to read in alphabet definitions from all files
/// alphabet.*.xml at startup (realization) time; it creates one CAlphInfo
/// object per alphabet at this time, and stores them in a map from AlphID
/// string until shutdown/destruction. (CAlphIO is a friend of CAlphInfo,
/// so can create/manipulate instances.)
class Dasher::CAlphIO : public AbstractXMLParser {
  public:
    CAlphIO(CMessageDisplay* pMsgs);
    virtual ~CAlphIO();

    virtual bool Parse(pugi::xml_document& document, const std::string filePath, bool bUser) override;

    void GetAlphabets(std::vector<std::string>* AlphabetList) const;
    const CAlphInfo* GetInfo(const std::string& AlphID) const;
    std::string GetDefault() const;

    /// True when the alphabet's full definition is parsed and loaded.
    /// (GetInfo silently falls back to Default for unknown IDs — callers
    /// deciding whether to load need this instead.)
    bool HasInfo(const std::string& AlphID) const;

    /// Record AlphID → filename from the cheap name index scan.
    void RememberFileName(const std::string& AlphID, const std::string& filename);

    /// Filename recorded by the index for an AlphID ("" when unknown).
    std::string FileNameFor(const std::string& AlphID) const;

    /// Build the name index: scan alphabet.*.xml and extract each file's
    /// root <alphabet name="..."> via a result-discarding pugixml parse —
    /// same parsing semantics as the full loader (entity references
    /// decoded, both quote styles, any prolog length) without building
    /// any CAlphInfo.
    void ScanNameIndex();

    /// Full-parse one alphabet file by (absolute or pattern) filename.
    bool LoadAlphabetFile(const std::string& filename);

  private:
    std::map<std::string, const CAlphInfo*> Alphabets; // map AlphabetID to AlphabetInfo.
    std::map<std::string, std::string> AlphabetFiles;  // name index: AlphID → filename
    static CAlphInfo*
    CreateDefault(); // Give the user an English alphabet rather than nothing if anything goes horribly wrong.

    void ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character& alphabet_character, SGroupInfo* parentGroup,
                            std::vector<ControlAction*>& DoActions, std::vector<ControlAction*>& UndoActions);
    SGroupInfo* ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet,
                                    SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors);
    void ReverseChildList(SGroupInfo*& pList);
    // Alphabet types:
    std::map<std::string, Options::AlphabetTypes> AlphabetStringToType;
};
/// @}
