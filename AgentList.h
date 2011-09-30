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

#ifndef _AGENT_LIST_H_
#define _AGENT_LIST_H_

#include "Agent.h"
#include "Event.h"

#include <algorithm>

/** Class for maintaining the set of all Agents involved in beat tracking a piece of music.
 */
class AgentList
{
public:
    typedef std::vector<Agent> Container;
    typedef Container::iterator iterator;

protected:
    Container list;

public:
    // expose some vector methods
    //!!! can we remove these again once the rest of AgentList is implemented?
    bool empty() const { return list.empty(); }
    Container::iterator begin() { return list.begin(); }
    Container::iterator end() { return list.end(); }
    void push_back(const Agent &a) { list.push_back(a); }

    /** Flag for choice between sum and average beat salience values for Agent scores.
     *  The use of summed saliences favours faster tempi or lower metrical levels. */
    static bool useAverageSalience;
	
    /** For the purpose of removing duplicate agents, the default JND of IBI */
    static const double DEFAULT_BI;
	
    /** For the purpose of removing duplicate agents, the default JND of phase */
    static const double DEFAULT_BT;

    /** Inserts newAgent into the list in ascending order of beatInterval */
    void add(Agent a) {
	add(a, true);
    } // add()/1

    /** Appends newAgent to list (sort==false), or inserts newAgent into the list
     *  in ascending order of beatInterval
     *  @param newAgent The agent to be added to the list
     *  @param sort Flag indicating whether the list is sorted or not
     */
    void add(Agent newAgent, bool sort){
	list.push_back(newAgent);
	if (sort) this->sort();
    } // add()/2

    /** Sorts the AgentList by increasing beatInterval. */
    void sort() {
	std::sort(list.begin(), list.end());
    } // sort()

    /** Removes the given item from the list.
     *  @param ptr Points to the Agent which is removed from the list
     */
    void remove(iterator itr) {
	list.erase(itr);
    } // remove()

protected:
    /** Removes Agents from the list which are duplicates of other Agents.
     *  A duplicate is defined by the tempo and phase thresholds
     *  thresholdBI and thresholdBT respectively.
     */
    void removeDuplicates() {
	sort();
	for (iterator itr = begin(); itr != end(); ++itr) {
	    if (itr->phaseScore < 0.0) // already flagged for deletion
		continue;
	    iterator itr2 = itr;
	    for (++itr2; itr2 != end(); ++itr2) {
		if (itr2->beatInterval - itr->beatInterval > DEFAULT_BI)
		    break;
		if (fabs(itr->beatTime - itr2->beatTime) > DEFAULT_BT)
		    continue;
		if (itr->phaseScore < itr2->phaseScore) {
		    itr->phaseScore = -1.0;	// flag for deletion
		    if (itr2->topScoreTime < itr->topScoreTime)
			itr2->topScoreTime = itr->topScoreTime;
		    break;
		} else {
		    itr2->phaseScore = -1.0;	// flag for deletion
		    if (itr->topScoreTime < itr2->topScoreTime)
			itr->topScoreTime = itr2->topScoreTime;
		}
	    }
	}
	for (iterator itr = begin(); itr != end(); ) {
	    if (itr->phaseScore < 0.0) {
		list.erase(itr);
	    } else {
		++itr;
	    }
	}
    } // removeDuplicates()

public:
    /** Perform beat tracking on a list of events (onsets).
     *  @param el The list of onsets (or events or peaks) to beat track
     */
    void beatTrack(EventList el) {
	beatTrack(el, -1.0);
    } // beatTrack()/1
	
    /** Perform beat tracking on a list of events (onsets).
     *  @param el The list of onsets (or events or peaks) to beat track.
     *  @param stop Do not find beats after <code>stop</code> seconds.
     */
    void beatTrack(EventList el, double stop) {
	EventList::iterator ei = el.begin();
	bool phaseGiven = !empty() && (begin()->beatTime >= 0); // if given for one, assume given for others
	while (ei != el.end()) {
	    Event ev = *ei;
	    ++ei;
	    if ((stop > 0) && (ev.time > stop))
		break;
	    bool created = phaseGiven;
	    double prevBeatInterval = -1.0;
	    // cc: Duplicate our list of agents, and scan through
	    // the (now immutable) copy.  This means we can safely
	    // add agents to our own list while scanning without
	    // disrupting our scan.
	    Container currentAgents = list;
	    for (Container::iterator ai = currentAgents.begin();
		 ai != currentAgents.end(); ++ai) {
		Agent currentAgent = *ai;
		if (currentAgent.beatInterval != prevBeatInterval) {
		    if ((prevBeatInterval>=0) && !created && (ev.time<5.0)) {
			// Create new agent with different phase
			Agent newAgent(prevBeatInterval);
			// This may add an agent to our list
			newAgent.considerAsBeat(ev, *this);
		    }
		    prevBeatInterval = currentAgent.beatInterval;
		    created = phaseGiven;
		}
		if (currentAgent.considerAsBeat(ev, *this))
		    created = true;
	    } // loop for each agent
	    removeDuplicates();
	} // loop for each event
    } // beatTrack()

    /** Finds the Agent with the highest score in the list, or NULL if beat tracking has failed.
     *  @return The Agent with the highest score
     */
    Agent *bestAgent() {
	double best = -1.0;
	Agent *bestAg = 0;
	for (iterator itr = begin(); itr != end(); ++itr) {
	    if (itr->events.empty()) continue;
	    double startTime = itr->events.begin()->time;
	    double conf = (itr->phaseScore + itr->tempoScore) /
		(useAverageSalience? (double)itr->beatCount: 1.0);
	    if (conf > best) {
		bestAg = &(*itr);
		best = conf;
	    }
	}
	return bestAg;
    } // bestAgent()


}; // class AgentList

#endif

