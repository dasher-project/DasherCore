#pragma once

#include "../AbstractXMLParser.h"

#include "../DasherTypes.h"
#include "AlphInfo.h"

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

	void GetAlphabets(std::vector< std::string >* AlphabetList) const;
	const CAlphInfo *GetInfo(const std::string& AlphID) const;
	std::string GetDefault() const;

private:
	std::map < std::string, const CAlphInfo* > Alphabets; // map AlphabetID to AlphabetInfo. 
	static CAlphInfo *CreateDefault();         // Give the user an English alphabet rather than nothing if anything goes horribly wrong.

	void ReadCharAttributes(pugi::xml_node xml_node, CAlphInfo::character& alphabet_character, SGroupInfo* parentGroup, std::vector<Action*>&
                            DoActions, std::vector<Action*>& UndoActions);
	SGroupInfo* ParseGroupRecursive(pugi::xml_node& group_node, CAlphInfo* CurrentAlphabet, SGroupInfo* previous_sibling, std::vector<SGroupInfo*> ancestors);
    void ReverseChildList(SGroupInfo*& pList);
	// Alphabet types:
	std::map<std::string, Options::AlphabetTypes> AlphabetStringToType;
};
/// @}
