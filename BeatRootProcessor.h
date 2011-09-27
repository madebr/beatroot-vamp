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

using std::vector;

class BeatRootProcessor
{
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

    /** The number of overlapping frames of audio data which have been read. */
    int frameCount;

    /** RMS amplitude of the current frame. */
    double frameRMS;

    /** Long term average frame energy (in frequency domain representation). */
    double ltAverage;

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
	
    /** The magnitude spectrum of the current frame. */
    vector<double> newFrame;

    /** The magnitude spectra of all frames, used for plotting the spectrogram. */
    vector<vector<double> > frames; //!!! do we need this? much cheaper to lose it if we don't
	
    /** The RMS energy of all frames. */
//    vector<double> energy; //!!! unused in beat tracking?
	
    /** The estimated onset times from peak-picking the onset
     * detection function(s). */
    vector<double> onsets;
	
    /** The estimated onset times and their saliences. */	
    EventList onsetList;

    /** Total number of audio frames if known, or -1 for live or compressed input. */
    int totalFrames;
	
    /** Flag for enabling or disabling debugging output */
    static bool debug;
	
    /** Flag for suppressing all standard output messages except results. */
    static bool silent;
	
    /** RMS frame energy below this value results in the frame being
     *  set to zero, so that normalisation does not have undesired
     *  side-effects. */
    static double silenceThreshold; //!!!??? energy of what? should not be static?
	
    /** For dynamic range compression, this value is added to the log
     *  magnitude in each frequency bin and any remaining negative
     *  values are then set to zero.
     */
    static double rangeThreshold; //!!! sim
	
    /** Determines method of normalisation. Values can be:<ul>
     *  <li>0: no normalisation</li>
     *  <li>1: normalisation by current frame energy</li>
     *  <li>2: normalisation by exponential average of frame energy</li>
     *  </ul>
     */
    static int normaliseMode;
	
    /** Ratio between rate of sampling the signal energy (for the
     * amplitude envelope) and the hop size */
//    static int energyOversampleFactor; //!!! not used?
	
public:

    /** Constructor: note that streams are not opened until the input
     *  file is set (see <code>setInputFile()</code>). */
    BeatRootProcessor() {
        frameRMS = 0;
        ltAverage = 0;
        frameCount = 0;
        hopSize = 0;
        fftSize = 0;
        hopTime = 0.010;	// DEFAULT, overridden with -h
        fftTime = 0.04644;	// DEFAULT, overridden with -f
        totalFrames = -1; //!!! not needed?
    } // constructor

protected:
    /** Allocates memory for arrays, based on parameter settings */
    void init() {
        hopSize = lrint(sampleRate * hopTime);
        fftSize = lrint(pow(2, lrint( log(fftTime * sampleRate) / log(2))));
        makeFreqMap(fftSize, sampleRate);
        prevFrame.clear();
        for (int i = 0; i < freqMapSize; i++) prevFrame.push_back(0);
        frameCount = 0;
        frameRMS = 0;
        ltAverage = 0;
        spectralFlux.clear();
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
        while (i <= crossoverBin)
            freqMap[i++] = i;
        while (i <= fftSize/2) {
            double midi = log(i*binWidth/440) / log(2) * 12 + 69;
            if (midi > 127)
                midi = 127;
            freqMap[i++] = crossoverBin + (int)lrint(midi) - crossoverMidi;
        }
        freqMapSize = freqMap[i-1] + 1;
    } // makeFreqMap()

    /** Processes a frame of audio data by first computing the STFT with a
     *  Hamming window, then mapping the frequency bins into a part-linear
     *  part-logarithmic array, then computing the spectral flux 
     *  then (optionally) normalising and calculating onsets.
     */
    void processFrame(const float *const *inputBuffers) {
        newFrame.clear();
        for (int i = 0; i < freqMapSize; i++) {
            newFrame.push_back(0);
        }
        double flux = 0;
        for (int i = 0; i <= fftSize/2; i++) {
            double mag = sqrt(inputBuffers[0][i*2] * inputBuffers[0][i*2] +
                              inputBuffers[0][i*2+1] * inputBuffers[0][i*2+1]);
            if (mag > prevFrame[i]) flux += mag - prevFrame[i];
            prevFrame[i] = mag;
            newFrame[freqMap[i]] += mag;
        }
        spectralFlux.push_back(flux);
        frames.push_back(newFrame);
//        for (int i = 0; i < freqMapSize; i++)
//            [frameCount][i] = newFrame[i];
/*
        int index = cbIndex - (fftSize - hopSize);
        if (index < 0)
            index += fftSize;
        int sz = (fftSize - hopSize) / energyOversampleFactor;
        for (int j = 0; j < energyOversampleFactor; j++) {
            double newEnergy = 0;
            for (int i = 0; i < sz; i++) {
                newEnergy += circBuffer[index] * circBuffer[index];
                if (++index == fftSize)
                    index = 0;
            }
            energy[frameCount * energyOversampleFactor + j] =
                newEnergy / sz <= 1e-6? 0: log(newEnergy / sz) + 13.816;
                }*/

        double decay = frameCount >= 200? 0.99:
            (frameCount < 100? 0: (frameCount - 100) / 100.0);

        //!!! uh-oh -- frameRMS has not been calculated (it came from time-domain signal) -- will always appear silent

        if (ltAverage == 0)
            ltAverage = frameRMS;
        else
            ltAverage = ltAverage * decay + frameRMS * (1.0 - decay);
        if (frameRMS <= silenceThreshold)
            for (int i = 0; i < freqMapSize; i++)
                frames[frameCount][i] = 0;
        else {
            if (normaliseMode == 1)
                for (int i = 0; i < freqMapSize; i++)
                    frames[frameCount][i] /= frameRMS;
            else if (normaliseMode == 2)
                for (int i = 0; i < freqMapSize; i++)
                    frames[frameCount][i] /= ltAverage;
            for (int i = 0; i < freqMapSize; i++) {
                frames[frameCount][i] = log(frames[frameCount][i]) + rangeThreshold;
                if (frames[frameCount][i] < 0)
                    frames[frameCount][i] = 0;
            }
        }
//			weightedPhaseDeviation();
//			if (debug)
//				System.err.printf("PhaseDev:  t=%7.3f  phDev=%7.3f  RMS=%7.3f\n",
//						frameCount * hopTime,
//						phaseDeviation[frameCount],
//						frameRMS);
        frameCount++;
    } // processFrame()

    /** Processes a complete file of audio data. */
    void processFile() {
/*
        while (pcmInputStream != null) {
            // Profile.start(0);
            processFrame();
            // Profile.log(0);
            if (Thread.currentThread().isInterrupted()) {
                System.err.println("info: INTERRUPTED in processFile()");
                return;
            }
        }
*/
//		double[] x1 = new double[phaseDeviation.length];
//		for (int i = 0; i < x1.length; i++) {
//			x1[i] = i * hopTime;
//			phaseDeviation[i] = (phaseDeviation[i] - 0.4) * 100;
//		}
//		double[] x2 = new double[energy.length];
//		for (int i = 0; i < x2.length; i++)
//			x2[i] = i * hopTime / energyOversampleFactor;
//		// plot.clear();
//		plot.addPlot(x1, phaseDeviation, Color.green, 7);
//		plot.addPlot(x2, energy, Color.red, 7);
//		plot.setTitle("Test phase deviation");
//		plot.fitAxes();

//		double[] slope = new double[energy.length];
//		double hop = hopTime / energyOversampleFactor;
//		Peaks.getSlope(energy, hop, 15, slope);
//		vector<Integer> peaks = Peaks.findPeaks(slope, (int)lrint(0.06 / hop), 10);
		
        double hop = hopTime;
        Peaks::normalise(spectralFlux);
        vector<int> peaks = Peaks::findPeaks(spectralFlux, (int)lrint(0.06 / hop), 0.35, 0.84, true);
        onsets.clear();
        onsets.resize(peaks.size(), 0);
        vector<int>::iterator it = peaks.begin();
        onsetList.clear();
        double minSalience = Peaks::min(spectralFlux);
        for (int i = 0; i < onsets.size(); i++) {
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

        //!!! This onsetList is then fed in to BeatTrackDisplay::beatTrack

    } // processFile()

}; // class AudioProcessor


#endif
