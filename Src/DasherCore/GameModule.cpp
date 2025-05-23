#include "GameModule.h"

#include <I18n.h>

#include "GameStatistics.h"
#include <sstream>
#include <cmath>

using namespace Dasher;

static SModuleSettings gameSets[] = {
  {SP_GAME_TEXT_FILE, T_STRING, -1, -1, -1, -1, _("Filename of sentences to enter")},
  {LP_GAME_HELP_DIST, T_LONG, 0, 4096, 2048, 128, _("Distance of sentence from center to decide user needs help")},
  {LP_GAME_HELP_TIME, T_LONG, 0, 10000, 1000, 100, _("Time for which user must need help before help drawn")},
  {BP_GAME_HELP_DRAW_PATH, T_BOOL, -1, -1, -1, -1, _("When we give help, show the shortest path to the target sentence?")},
};

CGameModule::CGameModule(CSettingsStore* pSettingsStore, Dasher::CDasherInterfaceBase *pInterface, CDasherView *pView, CDasherModel *pModel) 
: m_pSettingsStore(pSettingsStore), m_pInterface(pInterface),
m_pView(pView), m_iLastSym(-1), m_pModel(pModel),
m_y1(std::numeric_limits<myint>::min()), m_y2(std::numeric_limits<myint>::max()),
m_iTargetY(CDasherModel::ORIGIN_Y), m_uHelpStart(std::numeric_limits<unsigned long>::max()),
m_ulTotalTime(0), m_dTotalNats(0.0), m_uiTotalSyms(0), m_iFontSize(36)
{
    HandleViewChange(pView);

    m_pInterface->OnEditEvent.Subscribe(this, [this](CEditEvent::EditEventType type, const std::string& strText, CDasherNode* node)
    {
        HandleEditEvent(type, strText, node);
    });

    m_pModel->OnNodeChildrenCreated.Subscribe(this, [this](CDasherNode* node)
    {
       HandleNodePopulated(node); 
    });
}

bool CGameModule::GetSettings(SModuleSettings **sets, int *count) {
  *sets = gameSets;
  *count = sizeof(gameSets) / sizeof(gameSets[0]);
  return true;
}

CGameModule::~CGameModule()  {
    if (m_ulTotalTime) {
        //TODO make this a running commentary?
        std::ostringstream summary;
        summary << "Total time " << m_ulTotalTime; 
        summary << " nats " << m_dTotalNats << "=" << (m_dTotalNats*1000.0/m_ulTotalTime) << "/sec";
        summary << " chars " << m_uiTotalSyms << "=" << (m_uiTotalSyms/m_ulTotalTime) << "/sec";
        m_pInterface->Message(summary.str(),true);
    }
    m_pInterface->ClearAllContext();
    m_pInterface->OnEditEvent.Unsubscribe(this);
    m_pView->OnViewChanged.Unsubscribe(this);
    m_pView->OnGameNodeDraw.Unsubscribe(this);
    m_pModel->OnNodeChildrenCreated.Unsubscribe(this);
}

void CGameModule::HandleEditEvent(CEditEvent::EditEventType type, const std::string& strText, CDasherNode* node)
{
    if (!m_pAlph) return; //Game Mode currently not running
        const int iOffset(node->offset());
        switch(type) {
            // Added a new character (Stepped one node forward)
            case CEditEvent::EDIT_OUTPUT:
                if (iOffset == m_iLastSym+1 && iOffset < m_vTargetSymbols.size()) {
                    DASHER_ASSERT(m_strWrong == "");
                    if (strText == m_pAlph->GetText(m_vTargetSymbols[iOffset])) {
                        // User has entered correct text...
                        ++m_iLastSym;
                    } else m_strWrong = strText;
                } else {
                    DASHER_ASSERT(iOffset >= m_iLastSym+1);
                    m_strWrong += strText;
                }
                break;
            // Removed a character (Stepped one node back)
            case CEditEvent::EDIT_DELETE:
                if (iOffset == m_iLastSym) {
                    //seems they've just deleted the last _correct_ character they'd entered...
                    DASHER_ASSERT(evt->m_sText == m_pAlph->GetText(m_vTargetSymbols[m_iLastSym]));
                    --m_iLastSym;
                } else {
                    //just deleted previously-entered wrong text - hopefully they're heading in the right direction!
                    DASHER_ASSERT(m_strWrong.length() >= evt->m_sText.length());
                    DASHER_ASSERT(m_strWrong.substr(m_strWrong.length() - evt->m_sText.length()) == evt->m_sText);
                    m_strWrong = m_strWrong.substr(0,m_strWrong.length() - strText.length());
                }
                break;
            default:
                break;
        }
}

//Node populated...
void CGameModule::HandleNodePopulated(CDasherNode *pNode) {
  if (pNode->GetFlag(CDasherNode::NF_GAME) //if on game path, look for next/child node on path...
      && pNode->offset()+1 < m_vTargetSymbols.size())
    pNode->GameSearchChildren(m_vTargetSymbols[pNode->offset()+1]);
}


void CGameModule::HandleGameNodeDraw(CDasherNode*, myint y1, myint y2) {
  //game nodes form a single chain, i.e. are strictly nested.
  // we want the coordinates of the smallest (innermost) one about which we are told
  if (y1 > m_y1) m_y1 = y1;
  if (y2 < m_y2) m_y2 = y2;
}

void CGameModule::HandleViewChange(CDasherView *pView) {
    if (m_pView == pView || pView == nullptr) return; //Nothing changed

    if(m_pView) m_pView->OnViewChanged.Unsubscribe(this);
    if(m_pView) m_pView->OnGameNodeDraw.Unsubscribe(this);

    m_pView = pView;

    m_pView->OnViewChanged.Subscribe(this, [this](CDasherView* newView)
    {
        HandleViewChange(newView);
    });
    m_pView->OnGameNodeDraw.Subscribe(this, [this](CDasherNode* node, myint y1, myint y2)
    {
        HandleGameNodeDraw(node, y1, y2);
    });
}

void CGameModule::SetWordGenerator(const CAlphInfo *pAlph, CWordGeneratorBase *pWordGenerator) {
        m_pAlph = pAlph;
	m_pWordGenerator = pWordGenerator;
	if (!GenerateChunk()) {
    m_pInterface->Message("Game mode sentences file empty!",true);
    //this'll delete the 'this' pointer, so we'd better not do anything else afterwards!...
    m_pInterface->LeaveGameMode();
  }
}

void CGameModule::StartWriting(unsigned long lTime) {
  if (!m_ulSentenceStartTime) {
    m_ulSentenceStartTime = lTime;
    m_dSentenceStartNats = std::numeric_limits<double>::max();
  }
}

void CGameModule::DecorateView(unsigned long lTime, CDasherView *pView, CDasherModel *pModel) {

  if (m_dSentenceStartNats == std::numeric_limits<double>::max())
    m_dSentenceStartNats = pModel->GetNats();
  
  const myint iNewTarget((m_y1+m_y2)/2);
  m_vTargetY.push_back(iNewTarget);
  bool bDrawHelper;
  
  if (llabs(iNewTarget - CDasherModel::ORIGIN_Y) >= m_pSettingsStore->GetLongParameter(LP_GAME_HELP_DIST)) {
    //offscreen
    if (llabs(iNewTarget - CDasherModel::ORIGIN_Y) >= llabs(m_iTargetY - CDasherModel::ORIGIN_Y)) {
      //not decreasing
      if (m_uHelpStart == std::numeric_limits<unsigned long>::max())
        m_uHelpStart = lTime + m_pSettingsStore->GetLongParameter(LP_GAME_HELP_TIME);
    } else {
      //they're heading in the right direction
      if (m_uHelpStart >= lTime) //never displayed help, so assume they don't need it
        m_uHelpStart = std::numeric_limits<unsigned long>::max();
      //else, we were displaying help; keep so doing.
    }
    bDrawHelper = m_uHelpStart <= lTime;
  } else {
    //onscreen
    m_uHelpStart = std::numeric_limits<unsigned long>::max();
    bDrawHelper=false;
  }
  
  m_iTargetY = iNewTarget;
  if (bDrawHelper) {
    //draw a line along the y axis
    myint x[2], y[2];
    x[0] = x[1] = -100;
  
    const int lineWidth(m_pSettingsStore->GetLongParameter(LP_LINE_WIDTH));
    const CDasherView::DasherCoordScreenRegion visibleRegion = pView->VisibleRegion();

    if (m_y1 > visibleRegion.maxY) {
      //off the top! make arrow point straight up...
      y[1] = CDasherModel::MAX_Y;
      y[0] = y[1] - 400;
    } else if (m_y2 < visibleRegion.minY) {
      //off the bottom! make arrow point straight down...
      y[1] = 0;
      y[0] = 400;
    } else {
      //draw line parallel to that region of y-axis
      y[0] = m_y1; y[1] = m_y2;
      pView->DasherPolyline(x, y, 2, lineWidth, pView->GetNamedColor(NamedColor::gameGuide));
      //and make arrow horizontal, pointing to the midpoint
      x[0] = -400;
      y[0] = y[1] = m_iTargetY;
    }
    pView->DasherPolyarrow(x, y, 2, 3*lineWidth, pView->GetNamedColor(NamedColor::gameGuide), 0.2);
    
    if (m_pSettingsStore->GetBoolParameter(BP_GAME_HELP_DRAW_PATH)) DrawBrachistochrone(pView);
  }
  
  //reset location accumulators ready for next frame
  m_y1 = std::numeric_limits<myint>::min();
  m_y2 = std::numeric_limits<myint>::max();
  
  // Check if we've reached the end of a chunk
  if(m_iLastSym  == m_vTargetSymbols.size() - 1) {
    m_pInterface->Message(ComputeStats(m_vTargetY),true);
    m_vTargetY.clear(); //could preserve if samples not excessive...but is it meaningful (given restart)?
    m_pInterface->GetActiveInputMethod()->pause();
    m_ulTotalTime += (lTime - m_ulSentenceStartTime);
    m_dTotalNats += (pModel->GetNats() - m_dSentenceStartNats);
    m_uiTotalSyms += static_cast<unsigned int>(m_vTargetSymbols.size());
    if (!GenerateChunk()) {
      m_pInterface->Message("Game mode sentence file finished!",true);
      //note this deletes the 'this' pointer...
      m_pInterface->LeaveGameMode();
      //so better get out of here, fast!
      return;
    }
  }

  DrawText(pView);
}

void CGameModule::DrawBrachistochrone(CDasherView *pView) {
  // Plot a brachistochrone - the optimal path from the crosshair to the target
  // this is a circle, passing through both crosshair and target, centered on the y-axis
  const myint CenterY = ComputeBrachCenter();
  pView->DasherSpaceArc(CenterY, llabs(CenterY - m_iTargetY), CDasherModel::ORIGIN_X, CDasherModel::ORIGIN_Y, 0, m_iTargetY, pView->GetNamedColor(NamedColor::gameGuide), 2*(int)m_pSettingsStore->GetLongParameter(LP_LINE_WIDTH));
}

void CGameModule::DrawHelperArrow(Dasher::CDasherView* pView)
{
  // This plots a helpful pointer to the best direction to take to get to the target.
  // Probably too much floating point maths here, sort later.
  // Start of line is the crosshair location
  const int gameColor = 135; //Neon green. (!)
  const int noOfPoints = 10; // The curve will be made up of 9 straight segments...
  const myint m_iCrossX(CDasherModel::ORIGIN_X),m_iCrossY(CDasherModel::ORIGIN_Y);
  
  struct {
    myint iTargetY;
    myint iCenterY;
  } m_Target;
  m_Target.iTargetY = m_iTargetY;
  m_Target.iCenterY = ComputeBrachCenter();
  myint iX[noOfPoints];
  myint iY[noOfPoints];
  myint iLength;
  
  // Arrow starts at the cross hairs
  iX[0] = m_iCrossX;
  iY[0] = m_iCrossY;
  
  myint a = m_iCrossX/5;
  myint defaultlength = m_iCrossX - a ; 
  
  // ... then decide the length of the arrow...
  myint r = m_Target.iTargetY-m_Target.iCenterY; // radius of our circle (+ or -)
  
  if(m_Target.iTargetY < a && m_Target.iCenterY < m_iCrossY-defaultlength/2)
  {
    myint x = (myint) sqrt((double)(r*r-pow((double)(m_Target.iCenterY-a),2)));
    iLength = (myint) sqrt((double)(pow((double)(x-m_iCrossX),2)+pow((double)(a-m_iCrossY),2)));
  }
  else if(m_Target.iTargetY > 2*m_iCrossY-a && m_Target.iCenterY > m_iCrossY+defaultlength/2)
  {
    myint x = (myint) sqrt((double)(r*r-pow((double)(m_Target.iCenterY+a-2*m_iCrossY),2)));
    iLength = (myint) sqrt((double)(pow((double)(x-m_iCrossX),2)+pow((double)(a-m_iCrossY),2)));
  }
  else
    iLength = defaultlength;
  
  //...then calculate the points required...
  double angle = ((double)iLength/(double)r)/(double)noOfPoints;
  
  for(int n = 1; n < noOfPoints; ++n)
  {
    iX[n] = (myint) (cos(angle)*(iX[n-1]) - sin(angle)*(iY[n-1]-m_Target.iCenterY));
    iY[n] = (myint) (m_Target.iCenterY + sin(angle)*(iX[n-1]) + cos(angle)*(iY[n-1]-m_Target.iCenterY));
  }
  //...then plot it.
  pView->DasherPolyarrow(iX, iY, noOfPoints, m_pSettingsStore->GetLongParameter(LP_LINE_WIDTH)*4, pView->GetNamedColor(NamedColor::gameGuide), 1.414);
  
}

myint CGameModule::ComputeBrachCenter() {
  const myint iCrossX(CDasherModel::ORIGIN_X), iCrossY(CDasherModel::ORIGIN_Y);
  // This formula computes the Dasher Y Coordinate of the center of the circle on which
  // the dasher brachistochrone lies : iCenterY
  
  // It comes from the pythagorean relation: iCrossX^2 + (iCenterY - iCrossY)^2 = r^2
  // where r is the radius of the circle, r = abs(iTargetY-iCenterY)
  return static_cast<myint>(0.5*(double(iCrossX*iCrossX)/double(iCrossY-m_iTargetY)+iCrossY+m_iTargetY));
}

bool CGameModule::GenerateChunk() {
  m_iLastSym = -1;
  m_vTargetSymbols.clear();
  m_strWrong="";
  m_pWordGenerator->GetSymbols(m_vTargetSymbols);
  m_pInterface->ClearAllContext();
  m_ulSentenceStartTime = 0;
  if (m_vTargetSymbols.empty()) return false;
  ChunkGenerated();
  return true;
}
