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

#ifndef _BEATROOT_PROCESSOR_H_
#define _BEATROOT_PROCESSOR_H_

#include "Peaks.h"
#include "Event.h"
#include "BeatTracker.h"

#include <vector>
#include <cmath>

#ifdef DEBUG_BEATROOT
#include <iostream>
#endif

using std::vector;

class BeatRootProcessor
{
public:
    int getFFTSize() const { return fftSize; }
    int getHopSize() const { return hopSize; }

protected:
    /** Sample rate of audio */
    float sampleRate;
	
    /** Spacing of audio frames (determines the amount of overlap or
     *  skip between frames). This value is expressed in
     *  seconds. (Default = 0.020s) */
    double hopTime;

    /** The approximate size of an FFT frame in seconds. (Default =
     *  0.04644s).  The value is adjusted so that <code>fftSize</code>
     *  is always power of 2. */
    double fftTime;

    /** Spacing of audio frames in samples (see <code>hopTime</code>) */
    int hopSize;

    /** The size of an FFT frame in samples (see <code>fftTime</code>) */
    int fftSize;

    /** Spectral flux onset detection function, indexed by frame. */
    vector<double> spectralFlux;
	
    /** A mapping function for mapping FFT bins to final frequency bins.
     *  The mapping is linear (1-1) until the resolution reaches 2 points per
     *  semitone, then logarithmic with a semitone resolution.  e.g. for
     *  44.1kHz sampling rate and fftSize of 2048 (46ms), bin spacing is
     *  21.5Hz, which is mapped linearly for bins 0-34 (0 to 732Hz), and
     *  logarithmically for the remaining bins (midi notes 79 to 127, bins 35 to
     *  83), where all energy above note 127 is mapped into the final bin. */
    vector<int> freqMap;

    /** The number of entries in <code>freqMap</code>. Note that the length of
     *  the array is greater, because its size is not known at creation time. */
    int freqMapSize;

    /** The magnitude spectrum of the most recent frame.  Used for
     *  calculating the spectral flux. */
    vector<double> prevFrame;

    /** The estimated onset times from peak-picking the onset
     * detection function(s). */
    vector<double> onsets;
	
    /** The estimated onset times and their saliences. */	
    EventList onsetList;
    
    /** User-specifiable processing parameters. */
    AgentParameters agentParameters;
	
    /** Flag for suppressing all standard output messages except results. */
    static bool silent;
	
public:

    /** Constructor: note that streams are not opened until the input
     *  file is set (see <code>setInputFile()</code>). */
    BeatRootProcessor(float sr, AgentParameters parameters) :
        sampleRate(sr),
        hopTime(0.010),
        fftTime(0.04644),
        hopSize(0),
        fftSize(0),
        agentParameters(parameters)
    {
        hopSize = lrint(sampleRate * hopTime);
        fftSize = lrint(pow(2, lrint( log(fftTime * sampleRate) / log(2))));
        init();
    } // constructor

    void reset() {
        init();
    }

    /** Processes a frame of frequency-domain audio data by mapping
     *  the frequency bins into a part-linear part-logarithmic array,
     *  then computing the spectral flux then (optionally) normalising
     *  and calculating onsets.
     */
    void processFrame(const float *const *inputBuffers);

    /** Tracks beats once all frames have been processed by processFrame
     */
    EventList beatTrack();

protected:
    /** Allocates or re-allocates memory for arrays, based on parameter settings */
    void init() {
#ifdef DEBUG_BEATROOT
        std::cerr << "BeatRootProcessor::init()" << std::endl;
#endif
        makeFreqMap(fftSize, sampleRate);
        prevFrame.clear();
        for (int i = 0; i <= fftSize/2; i++) prevFrame.push_back(0);
        spectralFlux.clear();
        onsets.clear();
        onsetList.clear();
    } // init()

    /** Creates a map of FFT frequency bins to comparison bins.
     *  Where the spacing of FFT bins is less than 0.5 semitones, the mapping is
     *  one to one. Where the spacing is greater than 0.5 semitones, the FFT
     *  energy is mapped into semitone-wide bins. No scaling is performed; that
     *  is the energy is summed into the comparison bins. See also
     *  processFrame()
     */
    void makeFreqMap(int fftSize, float sampleRate) {
        freqMap.resize(fftSize/2+1);
        double binWidth = sampleRate / fftSize;
        int crossoverBin = (int)(2 / (pow(2, 1/12.0) - 1));
        int crossoverMidi = (int)lrint(log(crossoverBin*binWidth/440)/
                                       log(2) * 12 + 69);
        int i = 0;
        while (i <= crossoverBin && i <= fftSize/2) {
            freqMap[i] = i;
            ++i;
        }
        while (i <= fftSize/2) {
            double midi = log(i*binWidth/440) / log(2) * 12 + 69;
            if (midi > 127)
                midi = 127;
            freqMap[i] = crossoverBin + (int)lrint(midi) - crossoverMidi;
            ++i;
        }
        freqMapSize = freqMap[i-1] + 1;
    } // makeFreqMap()

}; // class AudioProcessor


#endif
