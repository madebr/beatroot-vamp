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

#ifndef _BEAT_TRACKER_H_
#define _BEAT_TRACKER_H_

#include "Event.h"
#include "Agent.h"
#include "AgentList.h"
#include "Induction.h"

using std::vector;

class BeatTracker
{
protected:
    /** beat data encoded as a list of Events */
    EventList beats;
	
    /** a list of onset events for passing to the tempo induction and beat tracking methods */
    EventList onsetList;
	
    /** the times of onsets (in seconds) */
    vector<double> onsets;

public:
    /** Constructor:
     *  @param b The list of beats
     */
    BeatTracker(EventList b) {
	beats = b;
    } // BeatTracker constructor

    /** Creates a new Event object representing a beat.
     *  @param time The time of the beat in seconds
     *  @param beatNum The index of the beat
     *  @return The Event object representing the beat
     */
    static Event newBeat(double time, int beatNum) {
	return Event(time, beatNum, 0);
    } // newBeat()

    /** Perform beat tracking.
     *  @param events The onsets or peaks in a feature list
     *  @return The list of beats, or an empty list if beat tracking fails
     */
    static EventList beatTrack(EventList events) {
	return beatTrack(events, EventList());
    }
	
    /** Perform beat tracking.
     *  @param events The onsets or peaks in a feature list
     *  @param beats The initial beats which are given, if any
     *  @return The list of beats, or an empty list if beat tracking fails
     */
    static EventList beatTrack(EventList events, EventList beats) {
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
	    agents.push_back(Agent(ioi));
	} else // tempo not given; use tempo induction
	    agents = Induction::beatInduction(events);
	if (!beats.empty())
	    for (AgentList::iterator itr = agents.begin(); itr != agents.end();
                 ++itr) {
		itr->beatTime = beatTime;
		itr->beatCount = count;
		itr->events = beats;
	    }
	agents.beatTrack(events, -1);
	Agent *best = agents.bestAgent();
	if (best) {
	    best->fillBeats(beatTime);
	    return best->events;
	}
	return EventList();
    } // beatTrack()/1
	
    /** Finds the mean tempo (as inter-beat interval) from an array of beat times
     *  @param d An array of beat times
     *  @return The average inter-beat interval
     */
    static double getAverageIBI(vector<double> d) {
	if (d.size() < 2)
	    return -1.0;
	return (d[d.size() - 1] - d[0]) / (d.size() - 1);
    } // getAverageIBI()
	
    /** Finds the median tempo (as inter-beat interval) from an array of beat times
     *  @param d An array of beat times
     *  @return The median inter-beat interval
     */
    static double getMedianIBI(vector<double> d) {
	if (d.size() < 2)
	    return -1.0;
	vector<double> ibi;
        ibi.resize(d.size()-1);
	for (int i = 1; i < d.size(); i++)
	    ibi[i-1] = d[i] - d[i-1];
        std::sort(ibi.begin(), ibi.end());
	if (ibi.size() % 2 == 0)
	    return (ibi[ibi.size() / 2] + ibi[ibi.size() / 2 - 1]) / 2;
	else
	    return ibi[ibi.size() / 2];
    } // getAverageIBI()
	
	
    // Various get and set methods
	
    /** @return the list of beats */
    EventList getBeats() {
	return beats;
    } // getBeats()

    /** @return the array of onset times */
    vector<double> getOnsets() {
	return onsets;
    } // getOnsets()

    /** Sets the onset times as a list of Events, for use by the beat tracking methods. 
     *  @param on The times of onsets in seconds
     */
    void setOnsetList(EventList on) {
	onsetList = on;
    } // setOnsetList()

    /** Sets the array of onset times, for displaying MIDI or audio input data.
     *  @param on The times of onsets in seconds
     */
    void setOnsets(vector<double> on) {
	onsets = on;
    } // setOnsets()

    /** Sets the list of beats.
     * @param b The list of beats
     */
    void setBeats(EventList b) {
	beats = b;
    } // setBeats()

}; // class BeatTrackDisplay


#endif

