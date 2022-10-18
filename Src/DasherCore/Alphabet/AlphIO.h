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

#include "../../Common/Common.h"
#include "../AbstractXMLParser.h"

#include "../DasherTypes.h"
#include "AlphInfo.h"

#include <map>
#include <pugixml.hpp>
#include <vector>
#include <utility>

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

	virtual bool Parse(pugi::xml_document& document, bool bUser) override;

	void GetAlphabets(std::vector< std::string >* AlphabetList) const;
	const CAlphInfo *GetInfo(const std::string& AlphID) const;
	std::string GetDefault() const;

private:
	std::map < std::string, const CAlphInfo* > Alphabets; // map AlphabetID to AlphabetInfo. 
	static CAlphInfo *CreateDefault();         // Give the user an English alphabet rather than nothing if anything goes horribly wrong.

	void ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character* alphabet_character);
	SGroupInfo* ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet, SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors);
    void ReverseChildList(SGroupInfo*& pList);
	// Alphabet types:
	std::map<std::string, Opts::AlphabetTypes> AlphabetStringToType;
};
/// @}
