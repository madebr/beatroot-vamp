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

#include "BeatTracker.h"

EventList BeatTracker::beatTrack(EventList events, EventList beats)
{
    AgentList agents;
    int count = 0;
    double beatTime = -1;
    if (!beats.empty()) {
	count = beats.size() - 1;
	EventList::iterator itr = beats.end();
	--itr;
	beatTime = itr->time;
    }
    if (count > 0) { // tempo given by mean of initial beats
	double ioi = (beatTime - beats.begin()->time) / count;
	agents.push_back(new Agent(ioi));
    } else // tempo not given; use tempo induction
	agents = Induction::beatInduction(events);
    if (!beats.empty())
	for (AgentList::iterator itr = agents.begin(); itr != agents.end();
	     ++itr) {
	    (*itr)->beatTime = beatTime;
	    (*itr)->beatCount = count;
	    (*itr)->events = beats;
	}
    agents.beatTrack(events, -1);
    Agent *best = agents.bestAgent();
    EventList results;
    if (best) {
	best->fillBeats(beatTime);
	results = best->events;
    }
    for (AgentList::iterator ai = agents.begin(); ai != agents.end(); ++ai) {
	delete *ai;
    }
    return results;
} // beatTrack()/1
	

