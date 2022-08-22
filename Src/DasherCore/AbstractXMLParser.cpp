/*
 *  AbstractXMLParser.cpp
 *  Dasher
 *
 *  Created by Alan Lawrence on 17/03/2011.
 *  Copyright 2011 Cavendish Laboratory. All rights reserved.
 *
 */

#include "AbstractXMLParser.h"

#include <fstream>

bool AbstractParser::ParseFile(const std::string& strPath, bool bUser)
{
	std::ifstream in(strPath.c_str(), std::ios::binary);
	bool res = Parse("file://" + strPath, in, bUser);
	in.close();
	return res;
}

bool AbstractXMLParser::Parse(const std::string& strDesc, std::istream& in, bool bUser)
{
	m_strDesc = strDesc;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load(in);

	if (!result) return false;

	return Parse(doc, bUser);
}
