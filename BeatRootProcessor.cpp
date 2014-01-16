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

#include "BeatRootProcessor.h"

bool
BeatRootProcessor::silent = true;

void BeatRootProcessor::processFrame(const float *const *inputBuffers) {
    double flux = 0;
    for (int i = 0; i <= fftSize/2; i++) {
        double mag = sqrt(inputBuffers[0][i*2] * inputBuffers[0][i*2] +
                          inputBuffers[0][i*2+1] * inputBuffers[0][i*2+1]);
        if (mag > prevFrame[i]) flux += mag - prevFrame[i];
        prevFrame[i] = mag;
    }
    
    spectralFlux.push_back(flux);
    
} // processFrame()

EventList BeatRootProcessor::beatTrack(EventList *unfilledReturn) {

#ifdef DEBUG_BEATROOT
    std::cerr << "Spectral flux:" << std::endl;
    for (int i = 0; i < spectralFlux.size(); ++i) {
        if ((i % 8) == 0) std::cerr << "\n";
        std::cerr << spectralFlux[i] << " ";
    }
#endif
		
    double hop = hopTime;
    Peaks::normalise(spectralFlux);
    vector<int> peaks = Peaks::findPeaks(spectralFlux, (int)lrint(0.06 / hop), 0.35, 0.84, true);
    onsets.clear();
    onsets.resize(peaks.size(), 0);
    vector<int>::iterator it = peaks.begin();
    onsetList.clear();
    double minSalience = Peaks::min(spectralFlux);
    for (int i = 0; i < (int)onsets.size(); i++) {
        int index = *it;
        ++it;
        onsets[i] = index * hop;
        Event e = BeatTracker::newBeat(onsets[i], 0);
//			if (debug)
//				System.err.printf("Onset: %8.3f  %8.3f  %8.3f\n",
//						onsets[i], energy[index], slope[index]);
//			e.salience = slope[index];	// or combination of energy + slope??
        // Note that salience must be non-negative or the beat tracking system fails!
        e.salience = spectralFlux[index] - minSalience;
        onsetList.push_back(e);
    }

#ifdef DEBUG_BEATROOT
    std::cerr << "Onsets: " << onsetList.size() << std::endl;
#endif

    return BeatTracker::beatTrack(agentParameters, onsetList, unfilledReturn);

} // processFile()

