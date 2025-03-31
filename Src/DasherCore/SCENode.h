



#ifndef __SCENODE_H__
#define __SCENODE_H__

/*Common Node Definition for Chinese Pinyin (possibly also Japanese) 
  Conversion Library and Dasher ConversionManager*/

#include <vector>

/// \ingroup Model
/// \{
class SCENode {
 public:
  SCENode();

  ~SCENode();

  void Ref() {
    ++m_iRefCount;
  };

  void Unref() {
    --m_iRefCount;
    
    if(m_iRefCount == 0) {
      delete this;
    }
  };
  
  const std::vector<SCENode *> &GetChildren() const {
    return m_vChildren;
  }
  void AddChild(SCENode *pChild) {
    m_vChildren.push_back(pChild);
    pChild->Ref();
  }

  char *pszConversion;

  //int IsHeadAndCandNum;
  //int CandIndex;
  int Symbol;
  //unsigned int SumPYProbStore;
  
  //int IsComplete;
  //int AcCharCount;  /*accumulative character count*/
  
  int NodeSize;
  
  //unsigned int HZFreq;
  //float HZProb;
 private:
  int m_iRefCount;

  std::vector<SCENode *> m_vChildren;
};
/// \}

#endif

