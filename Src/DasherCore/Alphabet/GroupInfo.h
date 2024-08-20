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

    std::string colorGroup;

    int iNumChildNodes;
    //This is purely descriptive/for debugging, except for MandarinDasher,
    // where it is used in training text to disambiguate which pinyin/pronunciation
    // (i.e. group) was used to produce a given target(chinese)-alphabet symbol
    std::string strName;
    void RecursiveDelete() {
        SGroupInfo* current = this;
        while(current != nullptr){
            if(current->pChild != nullptr){
                current->pChild->RecursiveDelete();
                current->pChild = nullptr;
            }
            SGroupInfo* next = current->pNext;
            delete current;
            current = next;
        }
    }
};


#endif
