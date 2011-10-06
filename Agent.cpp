/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
  Vamp feature extraction plugin for the BeatRoot beat tracker.

  Centre for Digital Music, Queen Mary, University of London.
  This file copyright 2011 Simon Dixon, Chris Cannam and QMUL.
    
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "Agent.h"
#include "BeatTracker.h"

double Agent::POST_MARGIN_FACTOR = 0.3;
double Agent::PRE_MARGIN_FACTOR = 0.15;
const double Agent::INNER_MARGIN = 0.040;
double Agent::MAX_CHANGE = 0.2;
double Agent::CONF_FACTOR = 0.5;
const double Agent::DEFAULT_CORRECTION_FACTOR = 50.0;
const double Agent::DEFAULT_EXPIRY_TIME = 10.0;

int Agent::idCounter = 0;

double Agent::innerMargin = 0.0;
double Agent::correctionFactor = 0.0;
double Agent::expiryTime = 0.0;
double Agent::decayFactor = 0.0;

bool Agent::considerAsBeat(Event e, AgentList &a) {
    double err;
    if (beatTime < 0) {	// first event
#ifdef DEBUG_BEATROOT
        std::cerr << "Ag#" << idNumber << ": accepting first event trivially at " << e.time << std::endl;
#endif
	accept(e, 0, 1);
	return true;
    } else {			// subsequent events
        EventList::iterator last = events.end();
        --last;
	if (e.time - last->time > expiryTime) {
#ifdef DEBUG_BEATROOT
            std::cerr << "Ag#" << idNumber << ": time " << e.time 
                      << " too late relative to " << last->time << " (expiry "
                      << expiryTime << "), giving up" << std::endl;
#endif
	    phaseScore = -1.0;	// flag agent to be deleted
	    return false;
	}
	double beats = nearbyint((e.time - beatTime) / beatInterval);
	err = e.time - beatTime - beats * beatInterval;
#ifdef DEBUG_BEATROOT
        std::cerr << "Ag#" << idNumber << ": time " << e.time << ", err " << err << " for beats " << beats << std::endl;
#endif
	if ((beats > 0) && (-preMargin <= err) && (err <= postMargin)) {
	    if (fabs(err) > innerMargin) {	// Create new agent that skips this
#ifdef DEBUG_BEATROOT
                std::cerr << "Ag#" << idNumber << ": creating another new agent" << std::endl;
#endif
		a.add(clone());	//  event (avoids large phase jump)
            }
	    accept(e, err, (int)beats);
	    return true;
	}
    }
    return false;
} // considerAsBeat()


void Agent::fillBeats(double start) {
    double prevBeat = 0, nextBeat, currentInterval, beats;
    EventList::iterator ei = events.begin();
    if (ei != events.end()) {
        EventList::iterator ni = ei;
	prevBeat = (++ni)->time;
    }
    for ( ; ei != events.end(); ) {
        EventList::iterator ni = ei;
	nextBeat = (++ni)->time;
	beats = nearbyint((nextBeat - prevBeat) / beatInterval - 0.01); //prefer slow
	currentInterval = (nextBeat - prevBeat) / beats;
	for ( ; (nextBeat > start) && (beats > 1.5); beats--) {
	    prevBeat += currentInterval;
            events.insert(ni, BeatTracker::newBeat(prevBeat, 0));
	}
	prevBeat = nextBeat;
        ei = ni;
    }
} // fillBeats()
