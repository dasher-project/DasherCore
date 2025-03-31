
//
// Created by Alan Lawrence on 17/06/2011.

#ifndef __SCREEN_GAME_MODULE_H__
#define __SCREEN_GAME_MODULE_H__

#include "GameModule.h"

namespace Dasher {
  class CScreenGameModule : public CGameModule {
  public:
    CScreenGameModule(CSettingsStore* pSettingsStore, CDasherInterfaceBase *pInterface,CDasherView *pView, CDasherModel *pModel);
    void HandleEditEvent(CEditEvent::EditEventType type, const std::string& strText, CDasherNode* node) override;
  protected:
    virtual void ChunkGenerated() override;
    virtual void DrawText(CDasherView *pView) override;
  private:
    std::string m_strEntered, m_strTarget;
    CDasherScreen::Label *m_pLabEntered, *m_pLabTarget, *m_pLabWrong;
    int m_iFirstSym, m_iLastSym;
  };
}

#endif
