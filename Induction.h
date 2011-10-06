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

#ifndef _INDUCTION_H_
#define _INDUCTION_H_

#include "Agent.h"
#include "AgentList.h"
#include "Event.h"

#include <vector>

#ifdef DEBUG_BEATROOT
#include <iostream>
#endif

using std::vector;

/** Performs tempo induction by finding clusters of similar
 *  inter-onset intervals (IOIs), ranking them according to the number
 *  of intervals and relationships between them, and returning a set
 *  of tempo hypotheses for initialising the beat tracking agents.
 */
class Induction
{
public:
    /** The maximum difference in IOIs which are in the same cluster */ 
    static double clusterWidth;
    
    /** The minimum IOI for inclusion in a cluster */
    static double minIOI;
	
    /** The maximum IOI for inclusion in a cluster */
    static double maxIOI;
	
    /** The minimum inter-beat interval (IBI), i.e. the maximum tempo
     *  hypothesis that can be returned.
     *  0.30 seconds == 200 BPM
     *  0.25 seconds == 240 BPM
     */
    static double minIBI; 

    /** The maximum inter-beat interval (IBI), i.e. the minimum tempo
     *  hypothesis that can be returned.
     *  1.00 seconds ==  60 BPM
     *  0.75 seconds ==  80 BPM
     *  0.60 seconds == 100 BPM
     */
    static double maxIBI;	//  60BPM	// was 0.75 =>  80
	
    /** The maximum number of tempo hypotheses to return */
    static int topN;
	
    /** Performs tempo induction (see JNMR 2001 paper by Simon Dixon for details). 
     *  @param events The onsets (or other events) from which the tempo is induced
     *  @return A list of beat tracking agents, where each is initialised with one
     *          of the top tempo hypotheses but no beats
     */
    static AgentList beatInduction(EventList events) {
	int i, j, b, bestCount;
	bool submult;
	int intervals = 0;			// number of interval clusters
	vector<int> bestn;// count of high-scoring clusters
	bestn.resize(topN);

	double ratio, err;
	int degree;
	int maxClusterCount = (int) ceil((maxIOI - minIOI) / clusterWidth);
	vector<double> clusterMean;
	clusterMean.resize(maxClusterCount);
	vector<int> clusterSize;
	clusterSize.resize(maxClusterCount);
	vector<int> clusterScore;
	clusterScore.resize(maxClusterCount);
		
	EventList::iterator ptr1, ptr2;
	Event e1, e2;
	ptr1 = events.begin();
	while (ptr1 != events.end()) {
	    e1 = *ptr1;
	    ++ptr1;
	    ptr2 = events.begin();
	    e2 = *ptr2;
	    ++ptr2;
	    while (e2 != e1 && ptr2 != events.end()) {
		e2 = *ptr2;
		++ptr2;
	    }
	    while (ptr2 != events.end()) {
		e2 = *ptr2;
		++ptr2;
		double ioi = e2.time - e1.time;
		if (ioi < minIOI)		// skip short intervals
		    continue;
		if (ioi > maxIOI)		// ioi too long
		    break;
		for (b = 0; b < intervals; b++)		// assign to nearest cluster
		    if (fabs(clusterMean[b] - ioi) < clusterWidth) {
			if ((b < intervals - 1) && (
				fabs(clusterMean[b+1] - ioi) <
				fabs(clusterMean[b] - ioi)))
			    b++;		// next cluster is closer
			clusterMean[b] = (clusterMean[b] * clusterSize[b] +ioi)/
			    (clusterSize[b] + 1);
			clusterSize[b]++;
			break;
		    }
		if (b == intervals) {	// no suitable cluster; create new one
		    if (intervals == maxClusterCount) {
//			System.err.println("Warning: Too many clusters");
			continue; // ignore this IOI
		    }
		    intervals++;
		    for ( ; (b>0) && (clusterMean[b-1] > ioi); b--) {
			clusterMean[b] = clusterMean[b-1];
			clusterSize[b] = clusterSize[b-1];
		    }
		    clusterMean[b] = ioi;
		    clusterSize[b] = 1;
		}
	    }
	}
	for (b = 0; b < intervals; b++)	// merge similar intervals
	    // TODO: they are now in order, so don't need the 2nd loop
	    // TODO: check BOTH sides before averaging or upper gps don't work
	    for (i = b+1; i < intervals; i++)
		if (fabs(clusterMean[b] - clusterMean[i]) < clusterWidth) {
		    clusterMean[b] = (clusterMean[b] * clusterSize[b] +
				      clusterMean[i] * clusterSize[i]) /
			(clusterSize[b] + clusterSize[i]);
		    clusterSize[b] = clusterSize[b] + clusterSize[i];
		    --intervals;
		    for (j = i+1; j <= intervals; j++) {
			clusterMean[j-1] = clusterMean[j];
			clusterSize[j-1] = clusterSize[j];
		    }
		}
	if (intervals == 0)
	    return AgentList();
	for (b = 0; b < intervals; b++)
	    clusterScore[b] = 10 * clusterSize[b];
	bestn[0] = 0;
	bestCount = 1;
	for (b = 0; b < intervals; b++)
	    for (i = 0; i <= bestCount; i++)
		if ((i < topN) && ((i == bestCount) ||
				   (clusterScore[b] > clusterScore[bestn[i]]))){
		    if (bestCount < topN)
			bestCount++;
		    for (j = bestCount - 1; j > i; j--)
			bestn[j] = bestn[j-1];
		    bestn[i] = b;
		    break;
		}
	for (b = 0; b < intervals; b++)	// score intervals
	    for (i = b+1; i < intervals; i++) {
		ratio = clusterMean[b] / clusterMean[i];
		submult = ratio < 1;
		if (submult)
		    degree = (int) nearbyint(1/ratio);
		else
		    degree = (int) nearbyint(ratio);
		if ((degree >= 2) && (degree <= 8)) {
		    if (submult)
			err = fabs(clusterMean[b]*degree - clusterMean[i]);
		    else
			err = fabs(clusterMean[b] - clusterMean[i]*degree);
		    if (err < (submult? clusterWidth : clusterWidth * degree)) {
			if (degree >= 5)
			    degree = 1;
			else
			    degree = 6 - degree;
			clusterScore[b] += degree * clusterSize[i];
			clusterScore[i] += degree * clusterSize[b];
		    }
		}
	    }

	AgentList a;
	for (int index = 0; index < bestCount; index++) {
	    b = bestn[index];
	    // Adjust it, using the size of super- and sub-intervals
	    double newSum = clusterMean[b] * clusterScore[b];
	    int newCount = clusterSize[b];
	    int newWeight = clusterScore[b];
	    for (i = 0; i < intervals; i++) {
		if (i == b)
		    continue;
		ratio = clusterMean[b] / clusterMean[i];
		if (ratio < 1) {
		    degree = (int) nearbyint(1 / ratio);
		    if ((degree >= 2) && (degree <= 8)) {
			err = fabs(clusterMean[b]*degree - clusterMean[i]);
			if (err < clusterWidth) {
			    newSum += clusterMean[i] / degree * clusterScore[i];
			    newCount += clusterSize[i];
			    newWeight += clusterScore[i];
			}
		    }
		} else {
		    degree = (int) nearbyint(ratio);
		    if ((degree >= 2) && (degree <= 8)) {
			err = fabs(clusterMean[b] - degree*clusterMean[i]);
			if (err < clusterWidth * degree) {
			    newSum += clusterMean[i] * degree * clusterScore[i];
			    newCount += clusterSize[i];
			    newWeight += clusterScore[i];
			}
		    }
		}
	    }
	    double beat = newSum / newWeight;
	    // Scale within range ... hope the grouping isn't ternary :(
	    while (beat < minIBI)		// Maximum speed
		beat *= 2.0;
	    while (beat > maxIBI)		// Minimum speed
		beat /= 2.0;
	    if (beat >= minIBI) {
		a.push_back(Agent(beat));
	    }
	}
#ifdef DEBUG_BEATROOT
        std::cerr << "Induction complete, returning " << a.size() << " agent(s)" << std::endl;
#endif
	return a;
    } // beatInduction()

protected:
    /** For variable cluster widths in newInduction().
     * @param low The lowest IOI allowed in the cluster
     * @return The highest IOI allowed in the cluster
     */
    static int top(int low) {
	return low + 25; // low/10;
    } // top()

}; // class Induction

#endif
