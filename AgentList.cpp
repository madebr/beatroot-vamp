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

#include "AgentList.h"

bool AgentList::useAverageSalience = false;
const double AgentList::DEFAULT_BI = 0.02;
const double AgentList::DEFAULT_BT = 0.04;

void AgentList::removeDuplicates() 
{
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
    int removed = 0;
    for (iterator itr = begin(); itr != end(); ) {
        if (itr->phaseScore < 0.0) {
            ++removed;
            list.erase(itr);
        } else {
            ++itr;
        }
    }
#ifdef DEBUG_BEATROOT
    if (removed > 0) {
        std::cerr << "removeDuplicates: removed " << removed << ", have "
                  << list.size() << " agent(s) remaining" << std::endl;
    }
    int n = 0;
    for (Container::iterator i = list.begin(); i != list.end(); ++i) {
        std::cerr << "agent " << n++ << ": time " << i->beatTime << std::endl;
    }
#endif
} // removeDuplicates()


void AgentList::beatTrack(EventList el, double stop)
{
    EventList::iterator ei = el.begin();
    bool phaseGiven = !empty() && (begin()->beatTime >= 0); // if given for one, assume given for others
    while (ei != el.end()) {
        Event ev = *ei;
        ++ei;
        if ((stop > 0) && (ev.time > stop))
            break;
        bool created = phaseGiven;
        double prevBeatInterval = -1.0;
        // cc: Duplicate our list of agents, and scan through the
        // copy.  This means we can safely add agents to our own
        // list while scanning without disrupting our scan.  Each
        // agent needs to be re-added to our own list explicitly
        // (since it is modified by e.g. considerAsBeat)
        Container currentAgents = list;
        list.clear();
        for (Container::iterator ai = currentAgents.begin();
             ai != currentAgents.end(); ++ai) {
            Agent currentAgent = *ai;
            if (currentAgent.beatInterval != prevBeatInterval) {
                if ((prevBeatInterval>=0) && !created && (ev.time<5.0)) {
#ifdef DEBUG_BEATROOT
                    std::cerr << "Creating a new agent" << std::endl;
#endif
                    // Create new agent with different phase
                    Agent newAgent(prevBeatInterval);
                    // This may add another agent to our list as well
                    newAgent.considerAsBeat(ev, *this);
                    add(newAgent);
                }
                prevBeatInterval = currentAgent.beatInterval;
                created = phaseGiven;
            }
            if (currentAgent.considerAsBeat(ev, *this))
                created = true;
            add(currentAgent);
        } // loop for each agent
        removeDuplicates();
    } // loop for each event
} // beatTrack()

Agent *AgentList::bestAgent()
{
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
#ifdef DEBUG_BEATROOT
    if (bestAg) {
        std::cerr << "Best agent: Ag#" << bestAg->idNumber << std::endl;
        std::cerr << "  Av-salience = " << best << std::endl;
    } else {
        std::cerr << "No surviving agent - beat tracking failed" << std::endl;
    }
#endif
    return bestAg;
} // bestAgent()

