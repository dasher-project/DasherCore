// AlphInfo.cpp
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

#include "AlphInfo.h"

using namespace Dasher;

CAlphInfo::CAlphInfo() {
  //Members of SGroupInfo:
    pChild=nullptr;
    pNext=nullptr;
    iStart=1;
    iEnd=1;
    iNumChildNodes = 0;

    m_iConversionID = None;
    m_strConversionTrainStart = "<";
    m_strConversionTrainStop = ">";
    m_strDefaultContext = ". ";
    m_strCtxChar = "§";
}

std::string CAlphInfo::escape(const std::string &ch) const {
    if ((m_strConversionTrainStart.length() && ch==m_strConversionTrainStart)
        || (m_strCtxChar.length() && ch==m_strCtxChar))
        return ch+ch;
  return ch;
}

CAlphInfo::~CAlphInfo() {
    pChild->RecursiveDelete();
    pNext->RecursiveDelete();

    for(auto action_vector : m_vCharacterDoActions)
    {
        for(auto a : action_vector) delete a;
        action_vector.clear();
    }
    for(auto action_vector : m_vCharacterUndoActions)
    {
        for(auto a : action_vector) delete a;
        action_vector.clear();
    }
}

void CAlphInfo::copyCharacterFrom(const CAlphInfo *other, int idx) {
    m_vCharacters.push_back(other->m_vCharacters[idx-1]);
}
