// AlphIO.cpp
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

#include "AlphIO.h"

#include <string>
#include <algorithm>

using namespace Dasher;

CAlphIO::CAlphIO(CMessageDisplay *pMsgs) : AbstractXMLParser(pMsgs) {
	Alphabets["Default"] = CreateDefault();

	AlphabetStringToType = {
		{"None", Opts::MyNone},
		{"Arabic", Opts::Arabic},
		{"Baltic", Opts::Baltic},
		{"CentralEurope", Opts::CentralEurope},
		{"ChineseSimplified", Opts::ChineseSimplified},
		{"ChineseTraditional", Opts::ChineseTraditional},
		{"Cyrillic", Opts::Cyrillic},
		{"Greek", Opts::Greek},
		{"Hebrew", Opts::Hebrew},
		{"Japanese", Opts::Japanese},
		{"Korean", Opts::Korean},
		{"Thai", Opts::Thai},
		{"Turkish", Opts::Turkish},
		{"VietNam", Opts::VietNam},
		{"Western", Opts::Western}
	};
}

SGroupInfo* CAlphIO::ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet, SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors)
{
	SGroupInfo* pNewGroup = new SGroupInfo();
    pNewGroup->iNumChildNodes = 0;
	pNewGroup->strName = group_node.attribute("name").as_string();
	pNewGroup->strLabel = group_node.attribute("label").as_string();
    pNewGroup->iColour = group_node.attribute("b").as_int(-1); //-1 marker for "none specified"; if so, will compute later

	pNewGroup->bVisible = !(ancestors.empty() && previous_sibling == nullptr); //by default, the first group in the alphabet is invisible
	const std::string visibility = group_node.attribute("visible").as_string();
	if(visibility == "yes" || visibility == "on")
	{
		pNewGroup->bVisible = true;
	}
	else if(visibility == "no" || visibility == "off")
	{
		pNewGroup->bVisible = false;
	}

    if (pNewGroup->iColour == -1 && pNewGroup->bVisible) {

		std::vector available_colors = {110,111,112};

		// Avoid same color as parent
		for(auto it = ancestors.rbegin(); it != ancestors.rend(); ++it)
		{
			if ((*it)->bVisible)
			{
				available_colors.erase(std::remove(available_colors.begin(), available_colors.end(), (*it)->iColour), available_colors.end());
				break;
			}
		}
		// Avoid same color as previous_sibling
		if(ancestors.size() >= 2 && previous_sibling != nullptr) available_colors.erase(std::remove(available_colors.begin(), available_colors.end(), previous_sibling->iColour), available_colors.end());

		pNewGroup->iColour = available_colors[0];
	}

	pNewGroup->pNext = previous_sibling;
    pNewGroup->pChild = nullptr;

	// symbols and groups
	pNewGroup->iStart = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
	std::vector<SGroupInfo*> new_ancestors(ancestors);
	new_ancestors.push_back(pNewGroup);
	SGroupInfo* previous_subgroup_sibling = nullptr;
	for(auto node : group_node.children())
	{
		// symbol
		if(std::strcmp(node.name(),"s") == 0)
		{
			CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1); //new char
			ReadCharAttributes(node, &CurrentAlphabet->m_vCharacters.back());
			pNewGroup->iNumChildNodes++;
		}

		// group
		if(std::strcmp(node.name(),"group") == 0)
		{
			SGroupInfo* newChildGroup = ParseGroupRecursive(node, CurrentAlphabet, previous_subgroup_sibling, new_ancestors);
			if(newChildGroup == nullptr) continue;
			pNewGroup->iNumChildNodes++;
			pNewGroup->pChild = newChildGroup;
			previous_subgroup_sibling = newChildGroup;
		}
	}

	pNewGroup->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
    if (pNewGroup->iEnd == pNewGroup->iStart) {
		//empty group. Delete it now, and remove from sibling chain
		SGroupInfo* parent = (ancestors.empty() ? CurrentAlphabet : ancestors.back());
		parent->pChild = pNewGroup->pNext; //the actually previous node
		delete pNewGroup;
		return nullptr;
    } else {
		//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
		ReverseChildList(pNewGroup->pChild);
    }

	return pNewGroup;
}

bool Dasher::CAlphIO::Parse(pugi::xml_document & document, bool bUser)
{
	pugi::xml_node alphabets = document.child("alphabets");

	const std::string language_code = alphabets.attribute("langcode").as_string();

	for (pugi::xml_node alphabet : alphabets)
    {
		if(std::strcmp(alphabet.name(), "alphabet") != 0) continue; // a non <alphabet ...> node

	    CAlphInfo* CurrentAlphabet = new CAlphInfo();
        CurrentAlphabet->Mutable = bUser;
		CurrentAlphabet->LanguageCode = language_code;
		CurrentAlphabet->AlphID = alphabet.attribute("name").as_string();
		CurrentAlphabet->m_strCtxChar = alphabet.attribute("escape").as_string();
		CurrentAlphabet->TrainingFile = alphabet.child("train").child_value();
		CurrentAlphabet->GameModeFile = alphabet.child("gamemode").child_value();
		CurrentAlphabet->PreferredColours = alphabet.child("palette").child_value();

		// orientation
		const std::string orientation_type = alphabet.child("orientation").attribute("type").as_string();
        if (orientation_type == "RL") {
            CurrentAlphabet->Orientation = Opts::RightToLeft;
        }
        else if (orientation_type == "TB") {
            CurrentAlphabet->Orientation = Opts::TopToBottom;
        }
        else if (orientation_type == "BT") {
            CurrentAlphabet->Orientation = Opts::BottomToTop;
        }
        else{
            CurrentAlphabet->Orientation = Opts::LeftToRight;
		}

		// Control character
		CurrentAlphabet->ControlCharacter = new CAlphInfo::character();
		ReadCharAttributes(alphabet.child("control"), CurrentAlphabet->ControlCharacter);

		// Convert character
		CurrentAlphabet->StartConvertCharacter = new CAlphInfo::character();
		ReadCharAttributes(alphabet.child("convert"), CurrentAlphabet->StartConvertCharacter);

		// protect (end convert) character
		CurrentAlphabet->EndConvertCharacter = new CAlphInfo::character();
		ReadCharAttributes(alphabet.child("protect"), CurrentAlphabet->EndConvertCharacter);

		// encoding
		const std::string encodingType = alphabet.child("encoding").attribute("type").as_string();
		CurrentAlphabet->Type = AlphabetStringToType[encodingType];

		// context
		CurrentAlphabet->m_strDefaultContext = alphabet.child("context").last_attribute().as_string(); //weird implementation but consistent with the original one

		// conversion mode
		const auto conversion_mode = alphabet.child("conversionmode");
		if(conversion_mode.type() != pugi::node_null){
			CurrentAlphabet->m_iConversionID = conversion_mode.attribute("id").as_int();
			CurrentAlphabet->m_strConversionTrainStart = conversion_mode.attribute("start").as_string();
			CurrentAlphabet->m_strConversionTrainStop = conversion_mode.attribute("stop").as_string();
		}

		// groups
		SGroupInfo* previous_sibling = nullptr;
		for(pugi::xml_node& child : alphabet.children())
		{
			if(std::strcmp(child.name(),"group") == 0){
				SGroupInfo* newGroup = ParseGroupRecursive(child, CurrentAlphabet, previous_sibling, {});
				CurrentAlphabet->iNumChildNodes++;
				CurrentAlphabet->pChild = newGroup; //always the last parsed child, as later reverse operation puts it as the first one
				previous_sibling = newGroup;
			}
		}

		// Paragraph character
		if(alphabet.child("paragraph").type() != pugi::node_null){
			CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1);
			CurrentAlphabet->iParagraphCharacter = static_cast<Dasher::symbol>(CurrentAlphabet->m_vCharacters.size());
			CurrentAlphabet->iNumChildNodes++;
			ReadCharAttributes(alphabet.child("paragraph"), &CurrentAlphabet->m_vCharacters.back());
			CurrentAlphabet->m_vCharacters.back().Text = "\n"; //This should be platform independent now.
		}

		// Space character
		if(alphabet.child("space").type() != pugi::node_null){
			CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1);
			CurrentAlphabet->iSpaceCharacter = static_cast<Dasher::symbol>(CurrentAlphabet->m_vCharacters.size());
			CurrentAlphabet->iNumChildNodes++;
			ReadCharAttributes(alphabet.child("space"), &CurrentAlphabet->m_vCharacters.back());
			if (CurrentAlphabet->m_vCharacters.back().Colour == -1) CurrentAlphabet->m_vCharacters.back().Colour = 9;
		}

		CurrentAlphabet->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
		//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
		ReverseChildList(CurrentAlphabet->pChild);

		Alphabets[CurrentAlphabet->AlphID] = CurrentAlphabet;
	}

	return true;
}

void CAlphIO::GetAlphabets(std::vector<std::string>* AlphabetList) const {
	AlphabetList->clear();

	for (auto [AlphabetID, Alphabet] : Alphabets){
		AlphabetList->push_back(Alphabet->AlphID);
	}
}

std::string CAlphIO::GetDefault() const {
	std::string DefaultExternalAlphabet = "English with limited punctuation";
	if(Alphabets.count(DefaultExternalAlphabet) != 0) {
		return DefaultExternalAlphabet;
	}

	return "Default";
}

const CAlphInfo *CAlphIO::GetInfo(const std::string &AlphabetID) const {
	auto it = Alphabets.find(AlphabetID);
	if (it == Alphabets.end()) //if we don't have the alphabet they ask for,
		it = Alphabets.find(GetDefault()); //give them default - it's better than nothing
	return it->second;
}

CAlphInfo *CAlphIO::CreateDefault() {
	// TODO I appreciate these strings should probably be in a resource file.
	// Not urgent though as this is not intended to be used. It's just a
	// last ditch effort in case file I/O totally fails.
	CAlphInfo &Default(*(new CAlphInfo()));
	Default.AlphID = "Default";
	Default.Type = Opts::Western;
	Default.Mutable = false;
	Default.TrainingFile = "training_english_GB.txt";
	Default.GameModeFile = "gamemode_english_GB.txt";
	Default.PreferredColours = "Default";
	std::string Chars = "abcdefghijklmnopqrstuvwxyz";

//   // Obsolete
//   Default.Groups.resize(1);
//   Default.Groups[0].Description = "Lower case Latin letters";
//   Default.Groups[0].Characters.resize(Chars.size());
//   Default.Groups[0].Colour = 0;
//   Default.m_pBaseGroup = 0;
//   for(unsigned int i = 0; i < Chars.size(); i++) {
//     Default.Groups[0].Characters[i].Text = Chars[i];
//     Default.Groups[0].Characters[i].Display = Chars[i];
//     Default.Groups[0].Characters[i].Colour = i + 10;
//   }
	// ---
	Default.pChild = 0;
	Default.Orientation = Opts::LeftToRight;

	//The following creates Chars.size()+2 actual character structs in the vector,
	// all initially blank. The extra 2 are for paragraph and space.
	Default.m_vCharacters.resize(Chars.size()+2);
	//fill in structs for characters in Chars...
	for(unsigned int i(0); i < Chars.size(); i++) {
		Default.m_vCharacters[i].Text = Chars[i];
		Default.m_vCharacters[i].Display = Chars[i];
		Default.m_vCharacters[i].Colour = i + 10;
	}

	//note iSpaceCharacter/iParagraphCharacter, as all symbol numbers, are one _more_
	// than their index into m_vCharacters... (as "unknown symbol" 0 does not appear in vector)
	Default.iParagraphCharacter = static_cast<Dasher::symbol>(Chars.size()+1);
	Default.m_vCharacters[Chars.size()].Display = "¶";
	Default.m_vCharacters[Chars.size()].Text = "\n"; //This should be platform independent now.
	Default.m_vCharacters[Chars.size()].Colour = 9;

	Default.iSpaceCharacter = static_cast<Dasher::symbol>(Chars.size()+2);
	Default.m_vCharacters[Chars.size()+1].Display = "_";
	Default.m_vCharacters[Chars.size()+1].Text = " ";
	Default.m_vCharacters[Chars.size()+1].Colour = 9;

	Default.ControlCharacter = new CAlphInfo::character();
	Default.ControlCharacter->Display = "Control";
	Default.ControlCharacter->Text = "";
	Default.ControlCharacter->Colour = 8;

	Default.iStart=1; Default.iEnd= static_cast<int>(Default.m_vCharacters.size())+1;
	Default.iNumChildNodes = static_cast<int>(Default.m_vCharacters.size());
	Default.pNext=Default.pChild=NULL;

	return &Default;
}

void CAlphIO::ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character* alphabet_character) {

	if(xml_node.type() == pugi::node_null) return;

	alphabet_character->Text = xml_node.attribute("t").as_string();
	alphabet_character->Display = xml_node.attribute("d").as_string();
	alphabet_character->Colour = xml_node.attribute("b").as_int(-1);
}

// Reverses the internal linked list for the given SGroupInfo
// input given GroupInfo eg. pointer to G_1 with [G_1->G_2->G_3->G_4->Null]
// result [G_4->G_3->G_2->G_1->Null]
void CAlphIO::ReverseChildList(SGroupInfo *&pList) {
	SGroupInfo *pFirst = pList;
	SGroupInfo *pPrev = NULL;
	while (pFirst) {
		SGroupInfo *pNext = pFirst->pNext;
		pFirst->pNext = pPrev;
		pPrev = pFirst;
		pFirst = pNext;
	}
	pList=pPrev;
}

CAlphIO::~CAlphIO() {
	for (auto [AlphabetID, Alphabet] : Alphabets) {
		delete Alphabet;
	}
}
