#include "DashIntfScreenMsgs.h"
#include "ScreenGameModule.h"

using namespace Dasher;

CDashIntfScreenMsgs::CDashIntfScreenMsgs(CSettingsStore *pSettingsStore)
	: CDashIntfSettings(pSettingsStore) {
}

void CDashIntfScreenMsgs::Message(const std::string &strText, bool bInterrupt) {
  //Just store the messages for Redraw...
  CDasherScreen::Label *lab = GetView()->Screen()->MakeLabel(strText);
  if (bInterrupt) {
    m_dqModalMessages.push_back(std::pair<CDasherScreen::Label*,bool>(lab,false));
    if (CInputFilter *fil=GetActiveInputMethod()) fil->pause();
  }
  else
    m_dqAsyncMessages.push_back(std::pair<CDasherScreen::Label*,unsigned long>(lab, 0));
}

bool CDashIntfScreenMsgs::FinishRender(unsigned long ulTime) {
  bool bMsgsChanged=false;
  //Finally any messages - newest that will fit at bottom, proceeding upwards.
  // Firstly clear any non-modal messages that have been onscreen for long enough
  while (!m_dqAsyncMessages.empty() && m_dqAsyncMessages.front().second && ulTime-m_dqAsyncMessages.front().second > static_cast<unsigned long>(GetLongParameter(LP_MESSAGE_TIME))) {
    delete m_dqAsyncMessages.front().first; //the Label
    m_dqAsyncMessages.pop_front(); // => stop displaying it
    bMsgsChanged=true;
  }
  CDasherScreen * const pScreen(GetView()->Screen());
  if (!m_dqAsyncMessages.empty() || !m_dqModalMessages.empty()) {
    screenint iY = pScreen->GetHeight();
    const screenint iMinY((iY*3)/4), iSW(pScreen->GetWidth());
    //still messages to display...first find out longest-ago N that will fit
    for (std::deque<std::pair<CDasherScreen::Label*, unsigned long> >::iterator it = m_dqAsyncMessages.begin(); it!=m_dqAsyncMessages.end() && iY>iMinY; it++) {
      if (it->second==0) {
        //reached a not-yet-displayed asynchronous message
        if (!m_dqModalMessages.empty()) break; //don't start displaying anything while there are modal msgs
        it->second = ulTime; //display message for first time
        bMsgsChanged=true;
      } 
      iY-=pScreen->TextSize(it->first, GetLongParameter(LP_MESSAGE_FONTSIZE)).second;
    }
    if (!m_dqModalMessages.empty()) {
      bool bDisp(m_dqModalMessages.front().second != 0); //displaying anything atm?
      for (std::deque<std::pair<CDasherScreen::Label*,unsigned long> >::iterator it=m_dqModalMessages.begin(); it!=m_dqModalMessages.end() && iY>iMinY; it++) {
        if (bDisp) {
          if (it->second==0) break; //don't start displaying more until previous dismissed
        } else {
          DASHER_ASSERT(it->second==0);
          it->second = ulTime;
          bMsgsChanged = true;
        }
        iY-=pScreen->TextSize(it->first, GetLongParameter(LP_MESSAGE_FONTSIZE)).second;
      }
    }
    //Now render messages proceeding downwards - non-modal first, then oldest first
    for (std::deque<std::pair<CDasherScreen::Label*, unsigned long> >::const_iterator it = m_dqAsyncMessages.begin(); it != m_dqAsyncMessages.end(); it++) {
      if (it->second==0) continue;
      std::pair<screenint,screenint> textDims = pScreen->TextSize(it->first, GetLongParameter(LP_MESSAGE_FONTSIZE));
      //black (5) rectangle:
      pScreen->DrawRectangle((iSW - textDims.first)/2, iY, (iSW+textDims.first)/2, iY+textDims.second, GetView()->GetNamedColor(NamedColor::infoTextBackground), ColorPalette::noColor, -1);
      //white (0) text for non-modal
      pScreen->DrawString(it->first, (iSW-textDims.first)/2, iY, GetLongParameter(LP_MESSAGE_FONTSIZE), GetView()->GetNamedColor(NamedColor::infoText));
      iY+=textDims.second;
    }

    for (std::deque<std::pair<CDasherScreen::Label*, unsigned long> >::const_iterator it = m_dqModalMessages.begin(); it != m_dqModalMessages.end(); it++) {
      if (it->second==0) continue;
      std::pair<screenint,screenint> textDims = pScreen->TextSize(it->first, GetLongParameter(LP_MESSAGE_FONTSIZE));
      //black (5) rectangle:
      pScreen->DrawRectangle((iSW - textDims.first)/2, iY, (iSW+textDims.first)/2, iY+textDims.second, GetView()->GetNamedColor(NamedColor::warningTextBackground), ColorPalette::noColor, -1);
      //yellow (111) for modal
      pScreen->DrawString(it->first, (iSW-textDims.first)/2, iY, GetLongParameter(LP_MESSAGE_FONTSIZE), GetView()->GetNamedColor(NamedColor::warningText));
      iY+=textDims.second;
    }
  }
  return bMsgsChanged;
}

void CDashIntfScreenMsgs::ChangeScreen(CDasherScreen *pNewScreen) {
  CDasherInterfaceBase::ChangeScreen(pNewScreen);
  for (std::deque<std::pair<CDasherScreen::Label*,unsigned long> >::iterator it=m_dqAsyncMessages.begin(); ; it++) {
    if (it==m_dqAsyncMessages.end()) it = m_dqModalMessages.begin();
    if (it==m_dqModalMessages.end()) break;
    const CDasherScreen::Label *pOldLabel(it->first);
    it->first = pNewScreen->MakeLabel(pOldLabel->m_strText, pOldLabel->m_iWrapSize);
    delete pOldLabel;
  }
}

void CDashIntfScreenMsgs::onUnpause(unsigned long lTime) {
  while (!m_dqModalMessages.empty()) {
    if (m_dqModalMessages.front().second) {
      //Message has been displayed; delete it
      delete m_dqModalMessages.front().first; //the label
      m_dqModalMessages.pop_front();
    } else {
      //there are more, not-yet displayed, modal messages!
      //These should be after any that were displayed (which have now been erased),
      // so do not unpause; next frame will render more messages instead.
      GetActiveInputMethod()->pause();
      return;
    }
  }
  CDasherInterfaceBase::onUnpause(lTime);
}

CGameModule *CDashIntfScreenMsgs::CreateGameModule() {
  return new CScreenGameModule(m_pSettingsStore, this, GetView(), m_pDasherModel);
}
