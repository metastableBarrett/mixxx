// QuantizeControl.cpp
// Created on Sat 5, 2011
// Author: pwhelan

#include <QtDebug>

#include "controlobject.h"
#include "configobject.h"
#include "controlpushbutton.h"
#include "cachingreader.h"
#include "engine/quantizecontrol.h"
#include "engine/enginecontrol.h"

QuantizeControl::QuantizeControl(QString group,
                                 ConfigObject<ConfigValue>* pConfig)
        : EngineControl(group, pConfig) {
    // Turn quantize OFF by default. See Bug #898213
    m_pCOQuantizeEnabled = new ControlPushButton(ConfigKey(group, "quantize"));
    m_pCOQuantizeEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pCONextBeat = new ControlObject(ConfigKey(group, "beat_next"));
    m_pCONextBeat->set(-1);
    m_pCOPrevBeat = new ControlObject(ConfigKey(group, "beat_prev"));
    m_pCOPrevBeat->set(-1);
    m_pCOClosestBeat = new ControlObject(ConfigKey(group, "beat_closest"));
    m_pCOClosestBeat->set(-1);
}

QuantizeControl::~QuantizeControl() {
    delete m_pCOQuantizeEnabled;
    delete m_pCONextBeat;
    delete m_pCOPrevBeat;
    delete m_pCOClosestBeat;
}

void QuantizeControl::trackLoaded(TrackPointer pTrack) {
    if (m_pTrack) {
        trackUnloaded(m_pTrack);
    }

    if (pTrack) {
        m_pTrack = pTrack;
        m_pBeats = m_pTrack->getBeats();
        connect(m_pTrack.data(), SIGNAL(beatsUpdated()),
                this, SLOT(slotBeatsUpdated()));
    }
}

void QuantizeControl::trackUnloaded(TrackPointer pTrack) {
    Q_UNUSED(pTrack);
    if (m_pTrack) {
        disconnect(m_pTrack.data(), SIGNAL(beatsUpdated()),
                   this, SLOT(slotBeatsUpdated()));
    }
    m_pTrack.clear();
    m_pBeats.clear();
    m_pCOPrevBeat->set(-1);
    m_pCONextBeat->set(-1);
    m_pCOClosestBeat->set(-1);
}

void QuantizeControl::slotBeatsUpdated() {
    if (m_pTrack) {
        m_pBeats = m_pTrack->getBeats();
    }
}

double QuantizeControl::process(const double dRate,
                                const double currentSample,
                                const double totalSamples,
                                const int iBufferSize) {
    Q_UNUSED(dRate);
    Q_UNUSED(totalSamples);
    Q_UNUSED(iBufferSize);

    if (!m_pBeats) {
        return kNoTrigger;
    }

    int iCurrentSample = currentSample;
    if (!even(iCurrentSample)) {
        iCurrentSample--;
    }

    double prevBeat = m_pCOPrevBeat->get();
    double nextBeat = m_pCONextBeat->get();
    double closestBeat = m_pCOClosestBeat->get();
    double currentClosestBeat = floor(m_pBeats->findClosestBeat(iCurrentSample));

    if (closestBeat != currentClosestBeat) {
        if (!even(static_cast<int>(currentClosestBeat))) {
            currentClosestBeat--;
        }
        m_pCOClosestBeat->set(currentClosestBeat);
    }

    if (prevBeat == -1 || nextBeat == -1 ||
        currentSample >= nextBeat || currentSample <= prevBeat) {
        // TODO(XXX) are the floor and even checks necessary?
        nextBeat = floor(m_pBeats->findNextBeat(iCurrentSample));
        prevBeat = floor(m_pBeats->findPrevBeat(iCurrentSample));

        if (!even(static_cast<int>(nextBeat)))
            nextBeat--;
        if (!even(static_cast<int>(prevBeat)))
            prevBeat--;

        m_pCONextBeat->set(nextBeat);
        m_pCOPrevBeat->set(prevBeat);
    }

    return kNoTrigger;
}
