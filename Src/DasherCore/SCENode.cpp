



#include "SCENode.h"

SCENode::SCENode() {
  m_iRefCount = 1;
}

SCENode::~SCENode() {
  // TODO: Delete string?

  for (std::vector<SCENode *>::iterator it = m_vChildren.begin(); it!=m_vChildren.end(); it++)
    (*it)->Unref();
}
