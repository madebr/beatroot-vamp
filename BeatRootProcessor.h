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

#include <vector>

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
    vector<int> spectralFlux;
	
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
    vector<double> energy; //!!! unused in beat tracking?
	
    /** The estimated onset times from peak-picking the onset
     * detection function(s). */
    vector<double> onsets;
	
    /** The estimated onset times and their saliences. */	
    //!!!EventList onsetList;
    vector<double> onsetList; //!!! corresponding to keyDown member of events in list

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
    static int energyOversampleFactor; //!!! not used?
	
public:

    /** Constructor: note that streams are not opened until the input
     *  file is set (see <code>setInputFile()</code>). */
    BeatRootProcessor() {
        cbIndex = 0;
        frameRMS = 0;
        ltAverage = 0;
        frameCount = 0;
        hopSize = 0;
        fftSize = 0;
        hopTime = 0.010;	// DEFAULT, overridden with -h
        fftTime = 0.04644;	// DEFAULT, overridden with -f
    } // constructor

protected:
	/** Allocates memory for arrays, based on parameter settings */
	void init() {
		hopSize = (int) Math.round(sampleRate * hopTime);
		fftSize = (int) Math.round(Math.pow(2,
				Math.round( Math.log(fftTime * sampleRate) / Math.log(2))));
		makeFreqMap(fftSize, sampleRate);
		int buffSize = hopSize * channels * 2;
		if ((inputBuffer == null) || (inputBuffer.length != buffSize))
			inputBuffer = new byte[buffSize];
		if ((circBuffer == null) || (circBuffer.length != fftSize)) {
			circBuffer = new double[fftSize];
			reBuffer = new double[fftSize];
			imBuffer = new double[fftSize];
			prevPhase = new double[fftSize];
			prevPrevPhase = new double[fftSize];
			prevFrame = new double[fftSize];
			window = FFT.makeWindow(FFT.HAMMING, fftSize, fftSize);
			for (int i=0; i < fftSize; i++)
				window[i] *= Math.sqrt(fftSize);
		}
		if (pcmInputStream == rawInputStream)
			totalFrames = (int)(pcmInputStream.getFrameLength() / hopSize);
		else
			totalFrames = (int) (MAX_LENGTH / hopTime);
		if ((newFrame == null) || (newFrame.length != freqMapSize)) {
			newFrame = new double[freqMapSize];
			frames = new double[totalFrames][freqMapSize];
		} else if (frames.length != totalFrames)
			frames = new double[totalFrames][freqMapSize];
		energy = new double[totalFrames*energyOversampleFactor];
		phaseDeviation = new double[totalFrames];
		spectralFlux = new double[totalFrames];
		frameCount = 0;
		cbIndex = 0;
		frameRMS = 0;
		ltAverage = 0;
	} // init()

	/** Closes the input stream(s) associated with this object. */
	void closeStreams() {
		if (pcmInputStream != null) {
			try {
				pcmInputStream.close();
				if (pcmInputStream != rawInputStream)
					rawInputStream.close();
				if (audioOut != null) {
					audioOut.drain();
					audioOut.close();
				}
			} catch (Exception e) {}
			pcmInputStream = null;
			audioOut = null;
		}
	} // closeStreams()

	/** Creates a map of FFT frequency bins to comparison bins.
	 *  Where the spacing of FFT bins is less than 0.5 semitones, the mapping is
	 *  one to one. Where the spacing is greater than 0.5 semitones, the FFT
	 *  energy is mapped into semitone-wide bins. No scaling is performed; that
	 *  is the energy is summed into the comparison bins. See also
	 *  processFrame()
	 */
	void makeFreqMap(int fftSize, float sampleRate) {
		freqMap = new int[fftSize/2+1];
		double binWidth = sampleRate / fftSize;
		int crossoverBin = (int)(2 / (Math.pow(2, 1/12.0) - 1));
		int crossoverMidi = (int)Math.round(Math.log(crossoverBin*binWidth/440)/
														Math.log(2) * 12 + 69);
		// freq = 440 * Math.pow(2, (midi-69)/12.0) / binWidth;
		int i = 0;
		while (i <= crossoverBin)
			freqMap[i++] = i;
		while (i <= fftSize/2) {
			double midi = Math.log(i*binWidth/440) / Math.log(2) * 12 + 69;
			if (midi > 127)
				midi = 127;
			freqMap[i++] = crossoverBin + (int)Math.round(midi) - crossoverMidi;
		}
		freqMapSize = freqMap[i-1] + 1;
	} // makeFreqMap()

	/** Calculates the weighted phase deviation onset detection function.
	 *  Not used.
	 *  TODO: Test the change to WPD fn */
	void weightedPhaseDeviation() {
		if (frameCount < 2)
			phaseDeviation[frameCount] = 0;
		else {
			for (int i = 0; i < fftSize; i++) {
				double pd = imBuffer[i] - 2 * prevPhase[i] + prevPrevPhase[i];
				double pd1 = Math.abs(Math.IEEEremainder(pd, 2 * Math.PI));
				phaseDeviation[frameCount] += pd1 * reBuffer[i];
				// System.err.printf("%7.3f   %7.3f\n", pd/Math.PI, pd1/Math.PI);
			}
		}
		phaseDeviation[frameCount] /= fftSize * Math.PI;
		double[] tmp = prevPrevPhase;
		prevPrevPhase = prevPhase;
		prevPhase = imBuffer;
		imBuffer = tmp;
	} // weightedPhaseDeviation()

	/** Reads a frame of input data, averages the channels to mono, scales
	 *  to a maximum possible absolute value of 1, and stores the audio data
	 *  in a circular input buffer.
	 *  @return true if a frame (or part of a frame, if it is the final frame)
	 *  is read. If a complete frame cannot be read, the InputStream is set
	 *  to null.
	 */
	bool getFrame() {
		if (pcmInputStream == null)
			return false;
		try {
			int bytesRead = (int) pcmInputStream.read(inputBuffer);
			if ((audioOut != null) && (bytesRead > 0))
				if (audioOut.write(inputBuffer, 0, bytesRead) != bytesRead)
					System.err.println("Error writing to audio device");
			if (bytesRead < inputBuffer.length) {
				if (!silent)
					System.err.println("End of input: " + audioFileName);
				closeStreams();
				return false;
			}
		} catch (IOException e) {
			e.printStackTrace();
			closeStreams();
			return false;
		}
		frameRMS = 0;
		double sample;
		switch(channels) {
			case 1:
				for (int i = 0; i < inputBuffer.length; i += 2) {
					sample = ((inputBuffer[i+1]<<8) |
							  (inputBuffer[i]&0xff)) / 32768.0;
					frameRMS += sample * sample;
					circBuffer[cbIndex++] = sample;
					if (cbIndex == fftSize)
						cbIndex = 0;
				}
				break;
			case 2: // saves ~0.1% of RT (total input overhead ~0.4%) :)
				for (int i = 0; i < inputBuffer.length; i += 4) {
					sample = (((inputBuffer[i+1]<<8) | (inputBuffer[i]&0xff)) +
							  ((inputBuffer[i+3]<<8) | (inputBuffer[i+2]&0xff)))
								/ 65536.0;
					frameRMS += sample * sample;
					circBuffer[cbIndex++] = sample;
					if (cbIndex == fftSize)
						cbIndex = 0;
				}
				break;
			default:
				for (int i = 0; i < inputBuffer.length; ) {
					sample = 0;
					for (int j = 0; j < channels; j++, i+=2)
						sample += (inputBuffer[i+1]<<8) | (inputBuffer[i]&0xff);
					sample /= 32768.0 * channels;
					frameRMS += sample * sample;
					circBuffer[cbIndex++] = sample;
					if (cbIndex == fftSize)
						cbIndex = 0;
				}
		}
		frameRMS = Math.sqrt(frameRMS / inputBuffer.length * 2 * channels);
		return true;
	} // getFrame()

	/** Processes a frame of audio data by first computing the STFT with a
	 *  Hamming window, then mapping the frequency bins into a part-linear
	 *  part-logarithmic array, then computing the spectral flux 
	 *  then (optionally) normalising and calculating onsets.
	 */
	void processFrame() {
		if (getFrame()) {
			for (int i = 0; i < fftSize; i++) {
				reBuffer[i] = window[i] * circBuffer[cbIndex];
				if (++cbIndex == fftSize)
					cbIndex = 0;
			}
			Arrays.fill(imBuffer, 0);
			FFT.magnitudePhaseFFT(reBuffer, imBuffer);
			Arrays.fill(newFrame, 0);
			double flux = 0;
			for (int i = 0; i <= fftSize/2; i++) {
				if (reBuffer[i] > prevFrame[i])
					flux += reBuffer[i] - prevFrame[i];
				newFrame[freqMap[i]] += reBuffer[i];
			}
			spectralFlux[frameCount] = flux;
			for (int i = 0; i < freqMapSize; i++)
				frames[frameCount][i] = newFrame[i];
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
						newEnergy / sz <= 1e-6? 0: Math.log(newEnergy / sz) + 13.816;
			}
			double decay = frameCount >= 200? 0.99:
						(frameCount < 100? 0: (frameCount - 100) / 100.0);
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
					frames[frameCount][i] = Math.log(frames[frameCount][i]) + rangeThreshold;
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
			double[] tmp = prevFrame;
			prevFrame = reBuffer;
			reBuffer = tmp;
			frameCount++;
			if ((frameCount % 100) == 0) {
				if (!silent) {
					System.err.printf("Progress: %1d %5.3f %5.3f\n", 
							frameCount, frameRMS, ltAverage);
					Profile.report();
				}
				if ((progressCallback != null) && (totalFrames > 0))
					progressCallback.setFraction((double)frameCount/totalFrames);
			}
		}
	} // processFrame()

	/** Processes a complete file of audio data. */
	void processFile() {
		while (pcmInputStream != null) {
			// Profile.start(0);
			processFrame();
			// Profile.log(0);
			if (Thread.currentThread().isInterrupted()) {
				System.err.println("info: INTERRUPTED in processFile()");
				return;
			}
		}

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
//		LinkedList<Integer> peaks = Peaks.findPeaks(slope, (int)Math.round(0.06 / hop), 10);
		
		double hop = hopTime;
		Peaks.normalise(spectralFlux);
		LinkedList<Integer> peaks = Peaks.findPeaks(spectralFlux, (int)Math.round(0.06 / hop), 0.35, 0.84, true);
		onsets = new double[peaks.size()];
		double[] y2 = new double[onsets.length];
		Iterator<Integer> it = peaks.iterator();
		onsetList = new EventList();
		double minSalience = Peaks.min(spectralFlux);
		for (int i = 0; i < onsets.length; i++) {
			int index = it.next();
			onsets[i] = index * hop;
			y2[i] = spectralFlux[index];
			Event e = BeatTrackDisplay.newBeat(onsets[i], 0);
//			if (debug)
//				System.err.printf("Onset: %8.3f  %8.3f  %8.3f\n",
//						onsets[i], energy[index], slope[index]);
//			e.salience = slope[index];	// or combination of energy + slope??
			// Note that salience must be non-negative or the beat tracking system fails!
			e.salience = spectralFlux[index] - minSalience;
			onsetList.add(e);
		}
		if (progressCallback != null)
			progressCallback.setFraction(1.0);
		if (doOnsetPlot) {
			double[] x1 = new double[spectralFlux.length];
			for (int i = 0; i < x1.length; i++)
				x1[i] = i * hopTime;
			plot.addPlot(x1, spectralFlux, Color.red, 4);
			plot.addPlot(onsets, y2, Color.green, 3);
			plot.setTitle("Spectral flux and onsets");
			plot.fitAxes();
		}
		if (debug) {
			System.err.printf("Onsets: %d\nContinue? ", onsets.length);
			readLine();
		}
	} // processFile()

	/** Reads a text file containing a list of whitespace-separated feature values.
	 *  Created for paper submitted to ICASSP'07.
	 *  @param fileName File containing the data
	 *  @return An array containing the feature values
	 */
	static double[] getFeatures(String fileName) {
		ArrayList<Double> l = new ArrayList<Double>();
		try {
			BufferedReader b = new BufferedReader(new FileReader(fileName));
			while (true) {
				String s = b.readLine();
				if (s == null)
					break;
				int start = 0;
				while (start < s.length()) {
					int len = s.substring(start).indexOf(' ');
					String t = null;
					if (len < 0)
						t = s.substring(start);
					else if (len > 0) {
						t = s.substring(start, start + len);
					}
					if (t != null)
						try {
							l.add(Double.parseDouble(t));
						} catch (NumberFormatException e) {
							System.err.println(e);
							if (l.size() == 0)
								l.add(new Double(0));
							else
								l.add(new Double(l.get(l.size()-1)));
						}
					start += len + 1;
					if (len < 0)
						break;
				}
			}
			double[] features = new double[l.size()];
			Iterator<Double> it = l.iterator();
			for (int i = 0; it.hasNext(); i++)
				features[i] = it.next().doubleValue();
			return features;
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			return null;
		} catch (IOException e) {
			e.printStackTrace();
			return null;
		} catch (NumberFormatException e) {
			e.printStackTrace();
			return null;
		}
	} // getFeatures()
	
	/** Reads a file of feature values, treated as an onset detection function,
	 *  and finds peaks, which are stored in <code>onsetList</code> and <code>onsets</code>.
	 * @param fileName The file of feature values
	 * @param hopTime The spacing of feature values in time
	 */
	void processFeatures(String fileName, double hopTime) {
		double hop = hopTime;
		double[] features = getFeatures(fileName);
		Peaks.normalise(features);
		LinkedList<Integer> peaks = Peaks.findPeaks(features, (int)Math.round(0.06 / hop), 0.35, 0.84, true);
		onsets = new double[peaks.size()];
		double[] y2 = new double[onsets.length];
		Iterator<Integer> it = peaks.iterator();
		onsetList = new EventList();
		double minSalience = Peaks.min(features);
		for (int i = 0; i < onsets.length; i++) {
			int index = it.next();
			onsets[i] = index * hop;
			y2[i] = features[index];
			Event e = BeatTrackDisplay.newBeat(onsets[i], 0);
			e.salience = features[index] - minSalience;
			onsetList.add(e);
		}
	} // processFeatures()

	/** Copies output of audio processing to the display panel. */
	void setDisplay(BeatTrackDisplay btd) {
		int energy2[] = new int[totalFrames*energyOversampleFactor];
		double time[] = new double[totalFrames*energyOversampleFactor];
		for (int i = 0; i < totalFrames*energyOversampleFactor; i++) {
			energy2[i] = (int) (energy[i] * 4 * energyOversampleFactor);
			time[i] = i * hopTime / energyOversampleFactor;
		}
		btd.setMagnitudes(energy2);
		btd.setEnvTimes(time);
		btd.setSpectro(frames, totalFrames, hopTime, 0);//fftTime/hopTime);
		btd.setOnsets(onsets);
		btd.setOnsetList(onsetList);
	} // setDisplay()
	
} // class AudioProcessor


#endif
