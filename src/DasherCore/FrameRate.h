// FrameRate.h
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002 David Ward
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SettingsStore.h"

namespace Dasher {
/// \ingroup Model
/// \{

/// keeps the framerate (LP_FRAMERATE / 100.0) up-to-date,
/// computes the Steps parameter,
/// computes RXmax - which controls the maximum rate of zooming in
class CFrameRate {
  public:
    CFrameRate(CSettingsStore* pSettingsStore);
    virtual ~CFrameRate();

    // Responds to a change to LP_FRAMERATE or LP_MAX_BITRATE
    //  by recomputing the Steps() parameter.
    virtual void HandleParameterChange(Parameter parameter);

    /// The number of frames, in which we will attempt to bring
    ///  the target location (under the cursor, or in dynamic button
    ///  modes) to the crosshair. See DJW thesis.
    int Steps() const { return m_iSteps; };

    ///
    /// Reset the framerate class
    /// TODO: Need to check semantics here
    /// Called from CDasherInterfaceBase::UnPause;
    ///
    void Reset_framerate(unsigned long Time) {
        m_iFrames = 0;
        m_iTime = Time;
        // An intentional pause must not be charged as a suspension when
        // recording resumes (and must NOT be zeroed: the first resumed
        // frame would otherwise fold the pause gap into the fresh window).
        m_iLastFrameTime = Time;
        m_iGuardedMs = 0; // a fresh session starts with a full budget
    }

    void RecordFrame(unsigned long Time);

  private:
    /// number of frames that have been sampled
    int m_iFrames;
    /// time at which first sampled frame was rendered
    unsigned long m_iTime;
    /// time at which the previous frame was recorded. A long gap between
    /// frames means the process was suspended or starved (GC, OS throttling
    /// of a background IME, debugger pause) — the wall-clock gap must not
    /// be folded into the framerate estimate (Dasher-Android #35).
    unsigned long m_iLastFrameTime = 0;
    /// total wall-clock time (ms) discarded by the suspension guard since
    /// the last COMPLETED measurement window. A stall is an outlier, but a
    /// sustained or bursty pattern of long gaps is a real slow cadence and
    /// must be measured — so at most ~1s of gap time is discardable per
    /// window; beyond that the gaps are recorded and the estimate adapts.
    /// (A counter reset by any single normal frame would freeze the
    /// estimate under bursty throttling: ≤3 long gaps + 1 normal frame,
    /// repeating — the classic timer-coalescing shape.) Deliberate blind
    /// spot: stall clusters interleaved with a full window of normal
    /// frames are forgiven entirely — repeated outliers never degrade
    /// the estimate, trading slow-cadence tracking for smoothness.
    unsigned long m_iGuardedMs = 0;
    static constexpr unsigned long kMaxGuardedMs = 1000;
    /// number of frames over which we will compute average framerate
    int m_iSamples;

    int m_iSteps;

    double m_dBitsAtLimX;

    CSettingsStore* m_pSettingsStore;
};
/// \}
} // namespace Dasher
