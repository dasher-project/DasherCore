// DasherInterfaceBase.cpp
//
// Copyright (c) 2008 The Dasher Team
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

#include "DasherInterfaceBase.h"

#include "DasherViewSquare.h"
#include "DasherScreen.h"
#include "DasherView.h"
#include "DasherInput.h"
#include "DasherModel.h"
#include "Event.h"
#include "NodeCreationManager.h"
#include "UserLog.h"
#include "BasicLog.h"
#include "GameModule.h"

// Input filters
#include "AlternatingDirectMode.h"
#include "ButtonMode.h"
#include "ClickFilter.h"
#include "PressFilter.h"
#include "CompassMode.h"
#include "DefaultFilter.h"
#include "OneButtonFilter.h"
#include "OneButtonDynamicFilter.h"
#include "OneDimensionalFilter.h"
#include "StylusFilter.h"
#include "TwoButtonDynamicFilter.h"
#include "TwoPushDynamicFilter.h"

// STL headers
#include <memory>
#include <sstream>
#include <algorithm>
#include <cctype>

static std::string alphabetIdToFilename(const std::string& alphId) {
    std::string lower = alphId;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    std::replace(lower.begin(), lower.end(), ' ', '.');
    return "alphabet." + lower + ".xml";
}

// Declare our global file logging object

#include "FileUtils.h"
#include "SmoothingFilter.h"
#include "DasherCore/FileLogger.h"
#ifndef NDEBUG
const eLogLevel g_iLogLevel = eLogLevel::logDEBUG;
const int g_iLogOptions = logTimeStamp | logDateStamp | logDeleteOldFile;
#else
const eLogLevel g_iLogLevel = eLogLevel::logNORMAL;
const int g_iLogOptions = logTimeStamp | logDateStamp;
#endif

using namespace Dasher;

CControlManager* CDasherInterfaceBase::GetControlManager() {
    return m_pNCManager ? m_pNCManager->GetControlManager() : nullptr;
}

CDasherInterfaceBase::CDasherInterfaceBase(CSettingsStore* pSettingsStore)
    : m_pDasherModel(std::make_unique<CDasherModel>()), m_pFramerate(std::make_unique<CFrameRate>(pSettingsStore)),
      m_pSettingsStore(pSettingsStore), m_pModuleManager(std::make_unique<CModuleManager>()) {

    m_pSettingsStore->OnParameterChanged.Subscribe(this, [this](Parameter p) { HandleParameterChange(p); });

    // Global logging object we can use from anywhere
    // Skip in low-memory mode (keyboard extension) to avoid sandbox write violations
    if (!m_bLowMemoryMode) {
        m_pGlobalApplicationLog = std::make_unique<CFileLogger>("dasher.log", g_iLogLevel, g_iLogOptions);
    }

    OnEditEvent.Subscribe(this, [this](CEditEvent::EditEventType type, const std::string& strText, CDasherNode*) {
        if (this->GetGameModule() || !m_pSettingsStore->GetBoolParameter(BP_SPEAK_WORDS) || !this->SupportsSpeech())
            return;

        if (type == CEditEvent::EDIT_OUTPUT) {
            if (!strText.empty() && std::isspace(strText[0])) {
                this->Speak(m_strCurrentWordToSpeak, false);
                m_strCurrentWordToSpeak = "";
            } else {
                m_strCurrentWordToSpeak += strText;
            }
        } else if (type == CEditEvent::EDIT_DELETE) {
            m_strCurrentWordToSpeak = m_strCurrentWordToSpeak.substr(
                0, std::max(static_cast<std::string::size_type>(0), m_strCurrentWordToSpeak.size() - strText.size()));
        }
    });
}

void CDasherInterfaceBase::Realize(unsigned long ulTime) {

    // if ChangeScreen has been called, we'll have created a view;
    //  otherwise, we still can't create a view, until we have a screen!
    DASHER_ASSERT(m_DasherScreen ? m_pDasherView != NULL : m_pDasherView == NULL);

    srand(ulTime);

    m_AlphIO = std::make_unique<CAlphIO>(this);
    if (m_bLowMemoryMode) {
        // Load only the selected alphabet (or default) to save ~5-10MB
        std::string alphId = m_pSettingsStore->GetStringParameter(SP_ALPHABET_ID);
        if (alphId.empty()) alphId = "English with limited punctuation";
        Dasher::FileUtils::ScanFiles(m_AlphIO.get(), alphabetIdToFilename(alphId));
        // If the specific file wasn't found, fall back to the default
        if (!m_AlphIO->GetInfo(alphId))
            Dasher::FileUtils::ScanFiles(m_AlphIO.get(), alphabetIdToFilename("English with limited punctuation"));
    } else {
        Dasher::FileUtils::ScanFiles(m_AlphIO.get(), "alphabet.*.xml");
    }

    m_ColorIO = std::make_unique<CColorIO>(this);
    Dasher::FileUtils::ScanFiles(m_ColorIO.get(), "color.*.xml");
    Dasher::FileUtils::ScanFiles(m_ColorIO.get(), "colour.*.xml");
    m_ColorIO->RelinkParents();

    ChangeView();

    ChangeColors();
    // Create the user logging object if we are suppose to.  We wait
    // until now so we have the real value of the parameter and not
    // just the default.

    // TODO: Sort out log type selection

    const int iUserLogLevel = m_pSettingsStore->GetLongParameter(LP_USER_LOG_LEVEL_MASK);
    if (iUserLogLevel == 10)
        m_pUserLog = std::make_unique<CBasicLog>(m_pSettingsStore, this);
    else if (iUserLogLevel > 0)
        m_pUserLog = std::make_unique<CUserLog>(m_pSettingsStore, this, iUserLogLevel);

    CreateModules();

    ChangeAlphabet(); // This creates the NodeCreationManager, the Alphabet,
    // and the tree of nodes in the model.

    CreateInput();
    CreateInputFilter();
    // we may have created a control manager already; in which case, we need
    //  it to realize there's now an inputfilter (which may provide more actions).
    //  So tell it the setting has changed...

    HandleParameterChange(LP_NODE_BUDGET);
    HandleParameterChange(BP_SPEAK_WORDS);

    // FIXME - need to rationalise this sort of thing.
    // InvalidateContext(true);
    ScheduleRedraw();

    // All the setup is done by now, so let the user log object know
    // that future parameter changes should be logged.
    if (m_pUserLog != NULL) m_pUserLog->InitIsDone();
}

CDasherInterfaceBase::~CDasherInterfaceBase() {
    // Deregistering here allows for reusing a settings instance
    m_pSettingsStore->OnParameterChanged.Unsubscribe(this);
    m_pSettingsStore->OnPreParameterChange.Unsubscribe(this);
    OnEditEvent.Unsubscribe(this); // Word Speak Event

    // //WriteTrainFileFull();???
    m_pDasherModel.reset(); // Needs to explicitly be deleted as else a crash occurs on close

    // Clean up cached lock label (created by Redraw when locked)
    delete m_pLockLabel;

    // When we destruct on shutdown, we'll output any detailed log file
    if (m_pUserLog) m_pUserLog->OutputFile();
}

void CDasherInterfaceBase::HandleParameterChange(Parameter parameter) {
    switch (parameter) {

    case LP_OUTLINE_WIDTH: // NOLINT(bugprone-branch-clone)
        ScheduleRedraw();
        break;
    case BP_DRAW_MOUSE:
        ScheduleRedraw();
        break;
    case BP_DRAW_MOUSE_LINE:
        ScheduleRedraw();
        break;
    case LP_ORIENTATION:
        m_pDasherView->SetOrientation(ComputeOrientation());
        ScheduleRedraw();
        break;
    case SP_ALPHABET_ID:
        ChangeAlphabet();
        ScheduleRedraw();
        break;
    case SP_COLOUR_ID:
        ChangeColors();
        ScheduleRedraw();
        break;
    case BP_PALETTE_CHANGE:
        if (m_pSettingsStore->GetBoolParameter(BP_PALETTE_CHANGE))
            m_pSettingsStore->SetStringParameter(SP_COLOUR_ID, m_pNCManager->GetAlphabet()->GetPalette());
        break;
    case LP_LANGUAGE_MODEL_ID:
        CreateNCManager();
        break;
    case LP_LINE_WIDTH: // NOLINT(bugprone-branch-clone)
        ScheduleRedraw();
        break;
    case LP_DASHER_FONTSIZE:
        ScheduleRedraw();
        break;
    case SP_INPUT_DEVICE:
        CreateInput();
        break;
    case SP_INPUT_FILTER:
        CreateInputFilter();
        ScheduleRedraw();
        break;
    case LP_MARGIN_WIDTH:
    case BP_NONLINEAR_Y:
    case LP_NONLINEAR_X:
    case LP_GEOMETRY:
    case LP_SHAPE_TYPE: // for platforms which actually have this as a GUI pref!
        ScheduleRedraw();
        break;
    case LP_NODE_BUDGET:
        m_defaultPolicy.reset(
            new AmortizedPolicy(m_pDasherModel.get(), m_pSettingsStore->GetLongParameter(LP_NODE_BUDGET)));
        break;
    case BP_CONTROL_MODE:
        // Rebuild control box first (deletes old CControlManager/templates),
        // then rebuild node tree — order is critical to avoid dangling
        // template pointers in live CContNodes during AddExtras().
        m_pNCManager->CreateControlBox();
        SetOffset(m_pDasherModel->GetOffset(), true);
        ScheduleRedraw();
        break;
    default:
        break;
    }
}

void CDasherInterfaceBase::EnterGameMode() {
    DASHER_ASSERT(m_pGameModule == NULL);
    if (CWordGeneratorBase* pWords = m_pNCManager->GetAlphabetManager()->GetGameWords()) {
        m_pGameModule = CreateGameModule();
        // m_pNCManager->updateControl();
        m_pGameModule->SetWordGenerator(m_pNCManager->GetAlphabet(), pWords);
        SetOffset(m_pDasherModel->GetOffset(), true);
    } else {
        /// TRANSLATORS: %s is the name of the alphabet; the string "GameTextFile"
        ///  refers to a setting name in gsettings or equivalent, and should not be translated.
        FormatMessage("Could not find game sentences file for %s - check alphabet definition, or override with "
                      "GameTextFile setting",
                      m_pNCManager->GetAlphabet()->GetID().c_str());
        m_pGameModule.reset(); // In case we are still in game mode
    }
}

void CDasherInterfaceBase::LeaveGameMode() {
    DASHER_ASSERT(m_pGameModule);
    m_pGameModule.reset(); // point at which we officially exit game mode
    // m_pNCManager->updateControl();
    SetBuffer(0);
}

void CDasherInterfaceBase::SetLockStatus(const std::string& strText, int iPercent) {
    std::string newMessage; // empty - what we want if iPercent==-1 (unlock)
    if (iPercent != -1) {
        std::ostringstream os;
        os << (strText.empty() ? "Training Dasher" : strText);
        if (iPercent) os << " " << iPercent << "%";
        newMessage = os.str();
    }
    if (newMessage != m_strLockMessage) {
        ScheduleRedraw();
        if (m_pLockLabel) {
            delete m_pLockLabel;
            m_pLockLabel = NULL;
        }
        m_strLockMessage = newMessage;
    }
}

void CDasherInterfaceBase::editOutput(const std::string& strText, CDasherNode* pCause) {
    OnEditEvent.Broadcast(CEditEvent::EDIT_OUTPUT, strText, pCause);
}

void CDasherInterfaceBase::editDelete(const std::string& strText, CDasherNode* pCause) {
    OnEditEvent.Broadcast(CEditEvent::EDIT_DELETE, strText, pCause);
}

void CDasherInterfaceBase::editConvert(CDasherNode* pCause) {
    OnEditEvent.Broadcast(CEditEvent::EDIT_CONVERT, "", pCause);
}

void CDasherInterfaceBase::editProtect(CDasherNode* pCause) {
    OnEditEvent.Broadcast(CEditEvent::EDIT_PROTECT, "", pCause);
}

void CDasherInterfaceBase::WriteTrainFileFull() {
    m_pNCManager->GetAlphabetManager()->WriteTrainFileFull(this);
}

void CDasherInterfaceBase::CreateNCManager() {

    if (!m_AlphIO || m_pSettingsStore->GetLongParameter(LP_LANGUAGE_MODEL_ID) == -1) return;

    // can't delete the old manager yet until we've deleted all its nodes...
    std::unique_ptr<CNodeCreationManager> oldMgr = std::move(m_pNCManager);
    // so create the new manager...
    m_pNCManager.reset(new CNodeCreationManager(m_pSettingsStore, this, m_AlphIO.get()));
    if (m_pSettingsStore->GetBoolParameter(BP_PALETTE_CHANGE)) {
        m_pSettingsStore->SetStringParameter(SP_COLOUR_ID, m_pNCManager->GetAlphabet()->GetPalette());
    }

    if (m_DasherScreen) {
        m_pNCManager->ChangeScreen(m_DasherScreen);
        // and start a new tree of nodes from it (retaining old offset -
        //  this will be a sensible default of 0 if no nodes previously existed).
        //  This deletes the old tree of nodes...
        SetOffset(m_pDasherModel->GetOffset(), true);
    } // else, if there is no screen, the model should not contain any nodes from the old NCManager. (Assert, somehow?)

    //...so now we can delete the old manager
    oldMgr.reset();
}

bool CDasherInterfaceBase::hasDone() {
    return (m_pSettingsStore->GetBoolParameter(BP_COPY_ALL_ON_STOP) && SupportsClipboard()) ||
           (m_pSettingsStore->GetBoolParameter(BP_SPEAK_ALL_ON_STOP) && SupportsSpeech());
}

void CDasherInterfaceBase::Done() {
    ScheduleRedraw();

    if (m_pUserLog != NULL) m_pUserLog->StopWriting((float)GetNats());

    if (m_pSettingsStore->GetBoolParameter(BP_COPY_ALL_ON_STOP) && SupportsClipboard()) {
        CopyToClipboard(GetAllContext());
    }
    if (m_pSettingsStore->GetBoolParameter(BP_SPEAK_ALL_ON_STOP) && SupportsSpeech()) {
        Speak(GetAllContext(), true);
    }
}

void CDasherInterfaceBase::CreateInput() {
    if (m_pInput) {
        m_pInput->Deactivate();
    }

    m_pInput = GetModuleManager()->GetInputDeviceByName(m_pSettingsStore->GetStringParameter(SP_INPUT_DEVICE));

    if (m_pInput == nullptr) m_pInput = m_pModuleManager->GetDefaultInputDevice();

    if (m_pInput) {
        m_pInput->Activate();
    }
}

void CDasherInterfaceBase::NewFrame(unsigned long iTime, bool bForceRedraw) {
    // Prevent NewFrame from being reentered. This can happen occasionally and
    // cause crashes.
    static bool bReentered = false;
    if (bReentered) {
#ifdef DEBUG
        std::cout << "CDasherInterfaceBase::NewFrame was re-entered" << std::endl;
#endif
        return;
    }
    bReentered = true;

    if (m_DasherScreen) {
        // ok, can draw _something_. Try and see what we can :).

        bool bBlit = false; // set to true if we actually render anything different i.e. that needs blitting to display

        if (isLocked() || !m_pDasherView) {
            // Hmmm. If we're locked, NewFrame is never actually called - the thread
            //  that would be rendering frames, is the same one doing the training.
            //  So the following is never actually executed atm, but may be a simple
            //  template if/when we ever implement multithreading widely/properly...
            const screenint iSW = m_DasherScreen->GetWidth(), iSH = m_DasherScreen->GetHeight();
            m_DasherScreen->DrawRectangle(0, 0, iSW, iSH, m_pDasherView->GetNamedColor(NamedColor::infoTextBackground),
                                          ColorPalette::noColor, 0); // fill in colour 0 = white
            unsigned int iSize(m_pSettingsStore->GetLongParameter(LP_MESSAGE_FONTSIZE));
            if (!m_pLockLabel) m_pLockLabel = m_DasherScreen->MakeLabel(m_strLockMessage, iSize);
            std::pair<screenint, screenint> dims = m_DasherScreen->TextSize(m_pLockLabel, iSize);
            m_DasherScreen->DrawString(m_pLockLabel, (iSW - dims.first) / 2, (iSH - dims.second) / 2, iSize,
                                       m_pDasherView->GetNamedColor(NamedColor::infoText));
            bBlit = true;
        } else {
            CExpansionPolicy* pol = m_defaultPolicy.get();

            // 1. Schedule any per-frame movement in the model...
            if (m_pInputFilter) {
                m_pInputFilter->Timer(iTime, m_pDasherView.get(), m_pInput, m_pDasherModel.get(), &pol);
            }
            // 2. Render...

            // If we've been told to render another frame via ScheduleRedraw,
            //  that's the same as passing in true to NewFrame.
            if (m_bRedrawScheduled) bForceRedraw = true;
            m_bRedrawScheduled = false;

            // Apply any movement that has been scheduled
            if (m_pDasherModel->NextScheduledStep()) {
                // yes, we moved...
                if (!m_bLastMoved) onUnpause(iTime);
                // ...so definitely need to render the nodes. We also make sure
                // to render at least one more frame - think that's a bit of policy
                // just to be on the safe side, and may not be strictly necessary...
                bForceRedraw = m_bRedrawScheduled = m_bLastMoved = true;
            } else {
                // no movement
                if (m_bLastMoved) bForceRedraw = true; // move into onPause() method if reqd
                m_bLastMoved = false;
            }
            // 2. Render nodes decorations, messages
            bBlit = Redraw(iTime, bForceRedraw, *pol);

            if (m_pUserLog != nullptr) {
                //(any) UserLogBase will have been watching output events to gather information
                // about symbols added/deleted; this tells it to apply that information at end-of-frame
                // (previously DashIntf gathered the info, and then passed it to the logger here).
                m_pUserLog->FrameEnded();
            }
        }
        if (FinishRender(iTime)) bBlit = true;
        if (bBlit) m_DasherScreen->Display();
    }

    bReentered = false;

    ExecuteDelayedActions();
}

void CDasherInterfaceBase::ExecuteDelayedActions() {
    for (auto& action : m_delayedActions)
        action();
    m_delayedActions.clear();
}

void CDasherInterfaceBase::onUnpause(unsigned long lTime) {
    // TODO When Game+UserLog modules are combined => reduce to just one call here
    if (m_pGameModule) m_pGameModule->StartWriting(lTime);
    if (m_pUserLog) m_pUserLog->StartWriting();
}

bool CDasherInterfaceBase::Redraw(unsigned long ulTime, bool bRedrawNodes, CExpansionPolicy& policy) {
    DASHER_ASSERT(m_pDasherView);

    // Draw the nodes
    if (bRedrawNodes) {
        if (m_pDasherModel) {
            m_pDasherModel->RenderToView(m_pDasherView.get(), policy);
            // if anything was expanded or collapsed render at least one more
            // frame after this
            if (policy.apply()) ScheduleRedraw();
        }
        if (m_pGameModule) {
            m_pGameModule->DecorateView(ulTime, m_pDasherView.get(), m_pDasherModel.get());
        }
    }

    // From here on, we'll use bRedrawNodes just to denote whether we need to blit the display...

    if (m_pInputFilter) {
        if (m_pInputFilter->DecorateView(m_pDasherView.get(), m_pInput)) bRedrawNodes = true;
    }

    return bRedrawNodes;
}

void CDasherInterfaceBase::ChangeAlphabet() {
    if (m_pSettingsStore->GetStringParameter(SP_ALPHABET_ID) == "") {
        m_pSettingsStore->SetStringParameter(SP_ALPHABET_ID, m_AlphIO->GetDefault());
        // This will result in ChangeAlphabet() being called again, so
        // exit from the first recursion
        return;
    }

    // In low-memory mode, load the requested alphabet on demand
    if (m_bLowMemoryMode && m_AlphIO) {
        std::string alphId = m_pSettingsStore->GetStringParameter(SP_ALPHABET_ID);
        if (!m_AlphIO->GetInfo(alphId)) {
            Dasher::FileUtils::ScanFiles(m_AlphIO.get(), alphabetIdToFilename(alphId));
        }
    }

    if (m_pNCManager) WriteTrainFileFull(); // can't/don't before creating first NCManager

    // Send a lock event

    // Lock Dasher to prevent changes from happening while we're training.

    CreateNCManager();
    if (m_pDasherView) m_pDasherView->SetOrientation(ComputeOrientation());
    // Apply options from alphabet

    //}
}

Options::ScreenOrientations CDasherInterfaceBase::ComputeOrientation() {
    Options::ScreenOrientations pref(Options::ScreenOrientations(m_pSettingsStore->GetLongParameter(LP_ORIENTATION)));
    if (pref != Options::AlphabetDefault) return pref;
    if (m_pNCManager) return m_pNCManager->GetAlphabet()->GetOrientation();
    // haven't created the NCManager yet, so not yet reached Realize, but must
    //  have been given Screen (to make View). Use default LR for now, as when
    //  we ChangeAlphabet, we'll update the view.
    return Options::LeftToRight;
}

void CDasherInterfaceBase::ChangeColors() {
    if (!m_ColorIO || !m_pDasherView) return;

    m_pDasherView->SetColorScheme(m_ColorIO->FindPalette(m_pSettingsStore->GetStringParameter(SP_COLOUR_ID)));
}

void CDasherInterfaceBase::ChangeScreen(CDasherScreen* NewScreen) {

    m_DasherScreen = NewScreen;

    if (m_pDasherView != 0) {
        m_pDasherView->ChangeScreen(NewScreen);
        ScreenResized(NewScreen);
    } else {
        // We can create the view as soon as we have a screen...
        ChangeView();
    }

    if (m_pNCManager) {
        m_pNCManager->ChangeScreen(m_DasherScreen);
        if (m_pDasherModel) SetOffset(m_pDasherModel->GetOffset(), true);
    }
}

void CDasherInterfaceBase::ScreenResized(CDasherScreen* pScreen) {
    DASHER_ASSERT(pScreen == m_DasherScreen);
    if (!m_pDasherView) return;
    m_pDasherView->ScreenResized(m_DasherScreen);

    // Really, would like to do a Redraw _immediately_, but this will have to do.
    ScheduleRedraw();
}

void CDasherInterfaceBase::ChangeView() {
    if (m_DasherScreen != 0 /*&& m_pDasherModel != 0*/) {
        CDasherView* pNewView = new CDasherViewSquare(m_pSettingsStore, m_DasherScreen, ComputeOrientation());
        // the previous sends an event to all listeners registered with it, but there aren't any atm!
        //  so send an event to tell them of the new view object _and_ get them to recompute coords:
        if (m_pDasherView) {
            m_pDasherView->OnViewChanged.Broadcast(pNewView);
        }
        m_pDasherView.reset(pNewView);
        ChangeColors();
    }
    ScheduleRedraw();
}

double CDasherInterfaceBase::GetCurCPM() {
    //
    return 0;
}

double CDasherInterfaceBase::GetCurFPS() {
    //
    return 0;
}

const CAlphInfo* CDasherInterfaceBase::GetActiveAlphabet() {
    return m_AlphIO->GetInfo(m_pSettingsStore->GetStringParameter(SP_ALPHABET_ID));
}

// int CDasherInterfaceBase::GetAutoOffset() {
//   if(m_pDasherView != 0) {
//     return m_pDasherView->GetAutoOffset();
//   }
//   return -1;
// }

double CDasherInterfaceBase::GetNats() const {
    if (m_pDasherModel)
        return m_pDasherModel->GetNats();
    else
        return 0.0;
}

void CDasherInterfaceBase::ResetNats() {
    if (m_pDasherModel) m_pDasherModel->ResetNats();
}

void CDasherInterfaceBase::ClearAllContext() {
    ctrlDelete(true, EDIT_FILE);
    ctrlDelete(false, EDIT_FILE);
    SetBuffer(0);
}

void CDasherInterfaceBase::ResetParameter(Parameter parameter) {
    m_pSettingsStore->ResetParameter(parameter);
}

// We need to be able to get at the UserLog object from outside the interface
CUserLogBase* CDasherInterfaceBase::GetUserLogPtr() {
    return m_pUserLog.get();
}

void CDasherInterfaceBase::KeyDown(unsigned long iTime, Keys::VirtualKey Key) {
    if (isLocked()) return;

    if (m_pInputFilter) {
        m_pInputFilter->KeyDown(iTime, Key, m_pDasherView.get(), m_pInput, m_pDasherModel.get());
    }

    if (m_pInput) {
        m_pInput->KeyDown(iTime, Key);
    }
}

void CDasherInterfaceBase::KeyUp(unsigned long iTime, Keys::VirtualKey Key) {
    if (isLocked()) return;

    if (m_pInputFilter) {
        m_pInputFilter->KeyUp(iTime, Key, m_pDasherView.get(), m_pInput, m_pDasherModel.get());
    }

    if (m_pInput) {
        m_pInput->KeyUp(iTime, Key);
    }
}

void CDasherInterfaceBase::CreateInputFilter() {
    if (m_pInputFilter) {
        m_pInputFilter->pause();
        m_pInputFilter->Deactivate();
        m_pInputFilter = nullptr;
    }

    m_pInputFilter = GetModuleManager()->GetInputMethodByName(m_pSettingsStore->GetStringParameter(SP_INPUT_FILTER));

    if (m_pInputFilter == nullptr) m_pInputFilter = m_pModuleManager->GetDefaultInputMethod();

    m_pInputFilter->Activate();
}

void CDasherInterfaceBase::CreateModules() {
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CDefaultFilter>(m_pSettingsStore, this, m_pFramerate.get(), _("Normal Control")), true);

    if (m_bLowMemoryMode) return;

    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CPressFilter>(m_pSettingsStore, this, m_pFramerate.get(), _("Press Mode")));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CSmoothingFilter>(m_pSettingsStore, this, m_pFramerate.get(), _("Smoothing Mode")));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<COneDimensionalFilter>(m_pSettingsStore, this, m_pFramerate.get()));
    GetModuleManager()->RegisterInputMethodModule(std::make_unique<CClickFilter>(m_pSettingsStore, this));
    GetModuleManager()->RegisterInputMethodModule(std::make_unique<COneButtonFilter>(m_pSettingsStore, this));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<COneButtonDynamicFilter>(m_pSettingsStore, this, m_pFramerate.get()));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CTwoButtonDynamicFilter>(m_pSettingsStore, this, m_pFramerate.get()));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CTwoPushDynamicFilter>(m_pSettingsStore, this, m_pFramerate.get()));
    // TODO: specialist factory for button mode
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CButtonMode>(m_pSettingsStore, this, true, _("Menu Mode")));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CButtonMode>(m_pSettingsStore, this, false, _("Direct Mode")));
    //  RegisterModule(new CDasherButtons(this, this, 4, 0, false,11, "Buttons 3"));
    GetModuleManager()->RegisterInputMethodModule(std::make_unique<CAlternatingDirectMode>(m_pSettingsStore, this));
    GetModuleManager()->RegisterInputMethodModule(std::make_unique<CCompassMode>(m_pSettingsStore, this));
    GetModuleManager()->RegisterInputMethodModule(
        std::make_unique<CStylusFilter>(m_pSettingsStore, this, m_pFramerate.get()));
    // WIP Temporary as too many segfaults! //RegisterModule(new CDemoFilter(this, this, m_pFramerate));
}

std::vector<std::string> CDasherInterfaceBase::GetPermittedValues(Parameter parameter) {
    std::vector<std::string> result;

    switch (parameter) {
    case SP_ALPHABET_ID:
        if (m_AlphIO) m_AlphIO->GetAlphabets(&result);
        break;
    case SP_COLOUR_ID:
        if (m_ColorIO) m_ColorIO->GetKnownPalettes(&result);
        break;
    case SP_INPUT_FILTER:
        m_pModuleManager->ListInputMethodModules(result);
        break;
    case SP_INPUT_DEVICE:
        m_pModuleManager->ListInputDeviceModules(result);
        break;
    default:
        break;
    }

    if (result.empty()) {
        auto it = Settings::parameter_defaults.find(parameter);
        if (it != Settings::parameter_defaults.end() && it->second.type == Settings::PARAM_STRING) {
            std::string current = m_pSettingsStore->GetStringParameter(parameter);
            if (!current.empty()) {
                result.push_back(current);
            }
        }
    }

    return result;
}

void CDasherInterfaceBase::SetOffset(int iOffset, bool bForce) {
    if (iOffset == m_pDasherModel->GetOffset() && !bForce) return;

    CDasherNode* pNode = m_pNCManager->GetAlphabetManager()->GetRoot(NULL, iOffset != 0, iOffset);
    if (GetGameModule()) pNode->SetFlag(CDasherNode::NF_GAME, true);
    m_pDasherModel->SetNode(pNode);

    ScheduleRedraw();
}

// Returns 0 on success, an error string on failure.
const char* CDasherInterfaceBase::ClSet(const std::string& strKey, const std::string& strValue) {
    return m_pSettingsStore->ClSet(strKey, strValue);
}

void CDasherInterfaceBase::ImportTrainingText(const std::string& strPath) {
    if (m_pNCManager) m_pNCManager->ImportTrainingText(strPath);
}

void CDasherInterfaceBase::WriteTrainFile(const std::string& filename, const std::string& strNewText) {
    Dasher::FileUtils::WriteUserDataFile(filename, strNewText, true);
};