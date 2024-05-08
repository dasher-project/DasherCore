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
#include <cstring>
#include <algorithm>

#include "../ColorIO.h"

using namespace Dasher;

CAlphIO::CAlphIO(CMessageDisplay *pMsgs) : AbstractXMLParser(pMsgs) {
	Alphabets["Default"] = CreateDefault();
}

SGroupInfo* CAlphIO::ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet, SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors)
{
	SGroupInfo* pNewGroup = new SGroupInfo();
    pNewGroup->iNumChildNodes = 0;
	pNewGroup->strName = group_node.attribute("name").as_string("");
	pNewGroup->strLabel = group_node.attribute("label").as_string("");
    pNewGroup->colorGroup = group_node.attribute("colorInfoName").as_string("");

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
		if(std::strcmp(node.name(),"node") == 0)
		{
			CurrentAlphabet->m_vCharacters.resize(CurrentAlphabet->m_vCharacters.size() + 1); //new char
			ReadCharAttributes(node, &CurrentAlphabet->m_vCharacters.back(), pNewGroup);
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
		//empty or defunct group. Delete it now, and remove from sibling chain
		SGroupInfo* parent = (ancestors.empty() ? CurrentAlphabet : ancestors.back());
		parent->pChild = pNewGroup->pNext; //the actually previous node
		delete pNewGroup;
		return nullptr;
    }

	//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
	ReverseChildList(pNewGroup->pChild);

	return pNewGroup;
}

bool Dasher::CAlphIO::Parse(pugi::xml_document & document, const std::string, bool bUser)
{
	pugi::xml_node alphabet = document.document_element();

	if(std::strcmp(alphabet.name(), "alphabet") != 0) return false; // a non <alphabet ...> node

	CAlphInfo* CurrentAlphabet = new CAlphInfo();
	CurrentAlphabet->AlphID = alphabet.attribute("name").as_string();
	CurrentAlphabet->TrainingFile = alphabet.attribute("trainingFilename").as_string();
	CurrentAlphabet->PreferredColors = alphabet.attribute("colorsName").as_string();

	// orientation
	const std::string orientation_type = alphabet.attribute("orientation").as_string("LR");
    if (orientation_type == "RL") {
        CurrentAlphabet->Orientation = Options::RightToLeft;
    }
    else if (orientation_type == "TB") {
        CurrentAlphabet->Orientation = Options::TopToBottom;
    }
    else if (orientation_type == "BT") {
        CurrentAlphabet->Orientation = Options::BottomToTop;
    }
    else{
        CurrentAlphabet->Orientation = Options::LeftToRight;
	}

	// conversion mode
	const std::string conversion_mode = alphabet.attribute("conversionMode").as_string("none");
    if (conversion_mode == "mandarin") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::Mandarin;
    }
    else if (conversion_mode == "routingContextInsensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextInsensitive;
    }
    else if (conversion_mode == "routingContextSensitive") {
        CurrentAlphabet->m_iConversionID = CAlphInfo::RoutingContextSensitive;
    }
    else{ //none
        CurrentAlphabet->m_iConversionID = CAlphInfo::None;
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

	CurrentAlphabet->iEnd = static_cast<int>(CurrentAlphabet->m_vCharacters.size()) + 1;
	//child groups were added (to linked list) in reverse order. Put them in (iStart/iEnd) order...
	ReverseChildList(CurrentAlphabet->pChild);

	Alphabets[CurrentAlphabet->AlphID] = CurrentAlphabet;

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
	CAlphInfo* Default = new CAlphInfo();
	Default->AlphID = "Default";
	Default->TrainingFile = "training_english_GB.txt";
	Default->PreferredColors = "Default";
	Default->Orientation = Options::LeftToRight;
	Default->colorGroup = "lowercase";

	Default->pChild = nullptr;

	std::string Chars = "abcdefghijklmnopqrstuvwxyz";
	Default->m_vCharacters.resize(Chars.size());
	//fill in structs for characters in Chars...
	for(int i = 0; i < Chars.size(); i++) {
		Default->m_vCharacters[i].Text = Chars[i];
		Default->m_vCharacters[i].Display = Chars[i];
		Default->m_vCharacters[i].ColorGroupOffset = i;
		Default->m_vCharacters[i].parentGroup = Default;
	}

	Default->iStart=1;
    Default->iEnd= static_cast<int>(Default->m_vCharacters.size())+1;
	Default->iNumChildNodes = static_cast<int>(Default->m_vCharacters.size());
	Default->pNext = nullptr;
	Default->pChild = nullptr;

	return Default;
}

void CAlphIO::ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character* alphabet_character, SGroupInfo* parentGroup) {

	if(xml_node.type() == pugi::node_null) return;

	alphabet_character->Display = xml_node.attribute("label").as_string();

	//Potential Unicode Symbol
	const auto textAction = xml_node.child("textCharAction");
	if(textAction.type() != pugi::node_null)
	{
	    const int unicodeSymbol = textAction.attribute("unicode").as_int(-1);
		alphabet_character->Text = (unicodeSymbol >= 0) ? std::string(1, static_cast<char>(unicodeSymbol)) : alphabet_character->Display;
	}

	alphabet_character->parentGroup = parentGroup;
	alphabet_character->ColorGroupOffset = parentGroup->iNumChildNodes;
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
