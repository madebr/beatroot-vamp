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

#ifndef _AGENT_H_
#define _AGENT_H_

#include "Event.h"

#include <cmath>

class AgentList;

/** Agent is the central class for beat tracking.
 *  Each Agent object has a tempo hypothesis, a history of tracked beats, and
 *  a score evaluating the continuity, regularity and salience of its beat track.
 */
class Agent
{
public:
    /** The maximum amount by which a beat can be later than the predicted beat time,
     *  expressed as a fraction of the beat period. */
    static double POST_MARGIN_FACTOR;

    /** The maximum amount by which a beat can be earlier than the predicted beat time,
     *  expressed as a fraction of the beat period. */
    static double PRE_MARGIN_FACTOR;
	
    /** The default value of innerMargin, which is the maximum time (in seconds) that a
     * 	beat can deviate from the predicted beat time without a fork occurring. */
    static const double INNER_MARGIN;
	
    /** The maximum allowed deviation from the initial tempo, expressed as a fraction of the initial beat period. */
    static double MAX_CHANGE;
		
    /** The slope of the penalty function for onsets which do not coincide precisely with predicted beat times. */
    static double CONF_FACTOR;
	
    /** The reactiveness/inertia balance, i.e. degree of change in the tempo, is controlled by the correctionFactor
     *  variable.  This constant defines its default value, which currently is not subsequently changed. The
     *  beat period is updated by the reciprocal of the correctionFactor multiplied by the difference between the
     *  predicted beat time and matching onset. */
    static const double DEFAULT_CORRECTION_FACTOR;
	
    /** The default value of expiryTime, which is the time (in seconds) after which an Agent that
     *  has no Event matching its beat predictions will be destroyed. */
    static const double DEFAULT_EXPIRY_TIME;

protected:
    /** The identity number of the next created Agent */
    static int idCounter;
	
    /** The maximum time (in seconds) that a beat can deviate from the predicted beat time
     *  without a fork occurring (i.e. a 2nd Agent being created). */
    static double innerMargin;

    /** Controls the reactiveness/inertia balance, i.e. degree of change in the tempo.  The
     *  beat period is updated by the reciprocal of the correctionFactor multiplied by the difference between the
     *  predicted beat time and matching onset. */
    static double correctionFactor;

    /** The time (in seconds) after which an Agent that
     *  has no Event matching its beat predictions will be destroyed. */
    static double expiryTime;
	
    /** For scoring Agents in a (non-existent) real-time version (otherwise not used). */
    static double decayFactor;

public:
    /** The size of the outer half-window before the predicted beat time. */
    double preMargin;

    /** The size of the outer half-window after the predicted beat time. */
    double postMargin;
	
    /** The Agent's unique identity number. */
    int idNumber;
	
    /** To be used in real-time version?? */
    double tempoScore;
	
    /** Sum of salience values of the Events which have been interpreted
     *  as beats by this Agent, weighted by their nearness to the predicted beat times. */
    double phaseScore;
	
    /** How long has this agent been the best?  For real-time version; otherwise not used. */
    double topScoreTime;
	
    /** The number of beats found by this Agent, including interpolated beats. */
    int beatCount;
	
    /** The current tempo hypothesis of the Agent, expressed as the beat period in seconds. */
    double beatInterval;

    /** The initial tempo hypothesis of the Agent, expressed as the beat period in seconds. */
    double initialBeatInterval;
	
    /** The time of the most recent beat accepted by this Agent. */
    double beatTime;
	
    /** The list of Events (onsets) accepted by this Agent as beats, plus interpolated beats. */
    EventList events;

    /** Constructor: the work is performed by init()
     *  @param ibi The beat period (inter-beat interval) of the Agent's tempo hypothesis.
     */
    Agent(double ibi) {
	init(ibi);
    } // constructor

    /** Copy constructor.
     *  @param clone The Agent to duplicate. */
    Agent(const Agent &clone) {
	idNumber = idCounter++;
	phaseScore = clone.phaseScore;
	tempoScore = clone.tempoScore;
	topScoreTime = clone.topScoreTime;
	beatCount = clone.beatCount;
	beatInterval = clone.beatInterval;
	initialBeatInterval = clone.initialBeatInterval;
	beatTime = clone.beatTime;
	events = EventList(clone.events);
	postMargin = clone.postMargin;
	preMargin = clone.preMargin;
    } // copy constructor

protected:
    /** Initialise all the fields of this Agent.
     *  @param ibi The initial tempo hypothesis of the Agent.
     */
    void init(double ibi) {
	innerMargin = INNER_MARGIN;
	correctionFactor = DEFAULT_CORRECTION_FACTOR;
	expiryTime = DEFAULT_EXPIRY_TIME;
	decayFactor = 0;
	beatInterval = ibi;
	initialBeatInterval = ibi;
	postMargin = ibi * POST_MARGIN_FACTOR;
	preMargin = ibi * PRE_MARGIN_FACTOR;
	idNumber = idCounter++;
	phaseScore = 0.0;
	tempoScore = 0.0;
	topScoreTime = 0.0;
	beatCount = 0;
	beatTime = -1.0;
	events.clear();
    } // init()


    double threshold(double value, double min, double max) {
	if (value < min)
	    return min;
	if (value > max)
	    return max;
	return value;
    }

public:
    /** Accept a new Event as a beat time, and update the state of the Agent accordingly.
     *  @param e The Event which is accepted as being on the beat.
     *  @param err The difference between the predicted and actual beat times.
     *  @param beats The number of beats since the last beat that matched an Event.
     */
    void accept(Event e, double err, int beats) {
	beatTime = e.time;
	events.push_back(e);
	if (fabs(initialBeatInterval - beatInterval -
		 err / correctionFactor) < MAX_CHANGE * initialBeatInterval)
	    beatInterval += err / correctionFactor;// Adjust tempo
	beatCount += beats;
	double conFactor = 1.0 - CONF_FACTOR * err /
	    (err>0? postMargin: -preMargin);
	if (decayFactor > 0) {
	    double memFactor = 1. - 1. / threshold((double)beatCount,1,decayFactor);
	    phaseScore = memFactor * phaseScore +
		(1.0 - memFactor) * conFactor * e.salience;
	} else
	    phaseScore += conFactor * e.salience;
    } // accept()

    /** The given Event is tested for a possible beat time. The following situations can occur:
     *  1) The Agent has no beats yet; the Event is accepted as the first beat.
     *  2) The Event is beyond expiryTime seconds after the Agent's last 'confirming' beat; the Agent is terminated.
     *  3) The Event is within the innerMargin of the beat prediction; it is accepted as a beat.
     *  4) The Event is within the postMargin's of the beat prediction; it is accepted as a beat by this Agent,
     *     and a new Agent is created which doesn't accept it as a beat.
     *  5) The Event is ignored because it is outside the windows around the Agent's predicted beat time.
     * @param e The Event to be tested
     * @param a The list of all agents, which is updated if a new agent is created.
     * @return Indicate whether the given Event was accepted as a beat by this Agent.
     */
    bool considerAsBeat(Event e, AgentList &a);

    /** Interpolates missing beats in the Agent's beat track, starting from the beginning of the piece. */
    void fillBeats() {
	fillBeats(-1.0);
    } // fillBeats()/0

    /** Interpolates missing beats in the Agent's beat track.
     *  @param start Ignore beats earlier than this start time 
     */
    void fillBeats(double start);

    // for sorting AgentList
    bool operator<(const Agent &a) const {
        return beatInterval < a.beatInterval;
    }

}; // class Agent

#endif