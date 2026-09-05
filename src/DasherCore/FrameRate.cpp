#include "FrameRate.h"
#include "DasherModel.h"
#include <algorithm>
#include <cmath>

using namespace Dasher;

CFrameRate::CFrameRate(CSettingsStore* pSettingsStore) : m_pSettingsStore(pSettingsStore) {

    // Sampling parameters...
    m_iFrames = 0;
    m_iSamples = 1;
    m_iTime = 0;

    // try and carry on from where we left off at last run
    CFrameRate::HandleParameterChange(LP_X_LIMIT_SPEED);
    // Sets m_dBitsAtLimX and m_iSteps
    m_pSettingsStore->OnParameterChanged.Subscribe(this, [this](const Parameter p) { HandleParameterChange(p); });
}

CFrameRate::~CFrameRate() {
    m_pSettingsStore->OnParameterChanged.Unsubscribe(this);
}

void CFrameRate::RecordFrame(unsigned long Time) {
    // Suspension guard (Dasher-Android #35): if the previous frame was a
    // long time ago, the process was stalled (GC pause, OS throttling of a
    // floating IME, occlusion, debugger). Frames are constant-pace zoom
    // steps, so the stall itself does no zoom catch-up — but folding the
    // wall-clock gap into this sampling window would collapse the
    // framerate estimate, shrink Steps(), and make every post-stall frame
    // zoom a large fraction at once ("lags, then the letters jump"). Treat
    // the gap as a suspension: drop the partial window and keep the
    // pre-stall estimate. 250ms ≈ 15 dropped frames at 60fps — generous
    // enough never to trigger on scheduler jitter.
    //
    // Budget, not counter: guarded gap time accumulates and is only
    // forgiven when a measurement window COMPLETES. A counter reset by any
    // single normal frame would freeze the estimate under bursty
    // throttling (≤3 long gaps + 1 normal frame repeating — the timer-
    // coalescing shape). Once ~1s has been discarded without a completed
    // window, long gaps are recorded and the estimate adapts to the real
    // cadence, however bursty. (Monotonicity is assumed; on platforms
    // where unsigned long wraps (~49.7 days of ms on LLP64), the first
    // post-wrap frame records normally and self-corrects via the decay.)
    if (m_iLastFrameTime != 0 && Time > m_iLastFrameTime && Time - m_iLastFrameTime > 250) {
        const unsigned long gap = Time - m_iLastFrameTime;
        if (m_iGuardedMs < kMaxGuardedMs) {
            m_iGuardedMs += gap;
            m_iTime = Time;
            m_iFrames = 0;
            m_iLastFrameTime = Time;
            return;
        }
        // Budget exhausted without a completed window: this is not an
        // interrupted cadence, it IS the cadence. Record the frame so
        // LP_FRAMERATE adapts — otherwise Steps() stays tuned for a
        // framerate the frontend no longer achieves and text entry stalls.
    }
    m_iLastFrameTime = Time;

    m_iFrames++;

    // Update values once enough samples have been collected
    if (m_iFrames == m_iSamples) {
        unsigned long m_iTime2 = Time;

        // If samples are collected in < 50ms, collect more
        if (m_iTime2 - m_iTime < 50) m_iSamples++;
        // And if it's taking longer than > 80ms, collect fewer, down to a
        // limit of 2
        else if (m_iTime2 - m_iTime > 80) {
            m_iSamples--;
            if (m_iSamples < 2) m_iSamples = 2;
        }

        // Calculate the framerate and reset framerate statistics for next
        // sampling period
        if (m_iTime2 - m_iTime > 0) {
            // A completed window is evidence of a live cadence: forgive the
            // suspension budget so future one-off stalls are guarded again.
            m_iGuardedMs = 0;
            double dFrNow = m_iFrames * 1000.0 / (m_iTime2 - m_iTime);
            // LP_FRAMERATE records a decaying average, smoothed 50:50 with previous value
            m_pSettingsStore->SetLongParameter(
                LP_FRAMERATE, long(m_pSettingsStore->GetLongParameter(LP_FRAMERATE) + (dFrNow * 100)) / 2);
            m_iTime = m_iTime2;
            m_iFrames = 0;

            // DASHER_TRACEOUTPUT("Fr %f Steps %d Samples %d Time2 %d\n", dFrNow, m_iSteps, m_iSamples, m_iTime2);
        }
    }
}

void CFrameRate::HandleParameterChange(Parameter parameter) {
    switch (parameter) {
    case LP_X_LIMIT_SPEED:
        m_dBitsAtLimX = (log(static_cast<double>(CDasherModel::MAX_Y)) -
                         log(2.0 * m_pSettingsStore->GetLongParameter(LP_X_LIMIT_SPEED))) /
                        log(2.0);
        // fallthrough
    case LP_MAX_BITRATE:
    case LP_FRAMERATE:
        // Calculate m_iSteps from the decaying-average framerate, as the number
        //  of steps that, at the X limit, will cause LP_MAX_BITRATE bits to be
        //  entered per second
        m_iSteps = std::max(1, (int)(m_pSettingsStore->GetLongParameter(LP_FRAMERATE) * m_dBitsAtLimX /
                                     m_pSettingsStore->GetLongParameter(LP_MAX_BITRATE)));
        break;
    default:
        break;
    }
}
