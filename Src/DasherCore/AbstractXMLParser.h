/*
 *  AbstractXMLParser.h
 *  Dasher
 *
 *  Created by Alan Lawrence on 17/03/2011.
 *  Copyright 2011 Cavendish Laboratory. All rights reserved.
 *
 */

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Messages.h"
#include "pugixml.hpp"

#include <string>

class AbstractParser
{
public:
	AbstractParser(CMessageDisplay* pMsgs) : m_pMsgs(pMsgs)
	{
	}
	virtual ~AbstractParser() = default;

	///Utility method: constructs an ifstream to read from the specified file,
	/// then calls Parse(string&,istream&,bool) with the description 'file://strPath'
	virtual bool ParseFile(const std::string& strPath, bool bUser);

	/// \param strDesc string to display to user to identify the source of this data,
	/// if there is an error. (Suggest: use a url, e.g. file://...)
	/// \param bUser if True, the file is from a user location (editable), false if from a
	/// system one. (Some subclasses treat the data differently according to which of these
	/// it is from.)
	virtual bool Parse(const std::string& strDesc, std::istream& in, bool bUser) = 0;

protected:
	///The MessageDisplay to use to inform the user. Subclasses should use this
	/// too for any (e.g. semantic) errors they may detect.
	CMessageDisplay* const m_pMsgs;
};

/// Basic wrapper over (PugiXML) XML Parser, handling file IO and wrapping C++
/// virtual methods over C callbacks. Subclasses must implement methods to
/// handle actual tags.
class AbstractXMLParser : public AbstractParser
{
public:
	bool Parse(const std::string& strDesc, std::istream& in, bool bUser) override;

	/**
	 * \brief Should be implemented by subclasses to parse files
	 * \param document Specifies the xml document, which is fully loaded and can be parsed
	 * \param bUser true, if this file lies in "user-land" and could potentially be written to
	 * \return true, iff parsing was successful 
	 */
	virtual bool Parse(pugi::xml_document& document, bool bUser) = 0;

protected:
	///Create an AbstractXMLParser which will use the specified MessageDisplay to
	/// inform the user of any errors.
	AbstractXMLParser(CMessageDisplay* pMsgs) : AbstractParser(pMsgs){}

	///Subclasses may call to get the description of the current file
	const std::string& GetDesc() { return m_strDesc; }

	std::string m_strDesc;
};
