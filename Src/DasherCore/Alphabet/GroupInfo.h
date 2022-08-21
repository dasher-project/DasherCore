#ifndef __GROUP_INFO_H__
#define __GROUP_INFO_H__

#include <string>

// Data Structure to store character groups inside alphabets
// Tree always starts with empty group
struct SGroupInfo {
  SGroupInfo *pChild; //Pointer to first child group
  SGroupInfo *pNext; //Pointer to next child of same parent
  std::string strLabel;
  ///lowest index of symbol that is in group
  int iStart;
  //one more than the highest index of a symbol in the group.
  // (iStart+1==iEnd => single character)
  int iEnd;
  int iColour;
  bool bVisible;
  int iNumChildNodes;
  //This is purely descriptive/for debugging, except for MandarinDasher,
  // where it is used in training text to disambiguate which pinyin/pronunciation
  // (i.e. group) was used to produce a given target(chinese)-alphabet symbol
  std::string strName;
  void RecursiveDelete() {
    for(SGroupInfo *t=this; t; ) {
      SGroupInfo *next = t->pNext;
      t->pChild->RecursiveDelete();
      delete t;
      t = next;
    }
  }
};


#endif
