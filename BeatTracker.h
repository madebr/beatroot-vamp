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
     *  @param unfilledReturn Pointer to list in which to return 
     *     un-interpolated beats, or NULL
     *  @return The list of beats, or an empty list if beat tracking fails
     */
    static EventList beatTrack(AgentParameters params, EventList events,
                               EventList *unfilledReturn) {
	return beatTrack(params, events, EventList(), unfilledReturn);
    }
	
    /** Perform beat tracking.
     *  @param events The onsets or peaks in a feature list
     *  @param beats The initial beats which are given, if any
     *  @param unfilledReturn Pointer to list in which to return
     *     un-interpolated beats, or NULL
     *  @return The list of beats, or an empty list if beat tracking fails
     */
    static EventList beatTrack(AgentParameters params,
                               EventList events, EventList beats,
                               EventList *unfilledReturn);
	
	
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

