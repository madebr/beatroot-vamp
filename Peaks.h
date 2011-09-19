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

#ifndef _BEATROOT_PEAKS_H_
#define _BEATROOT_PEAKS_H_

#include <vector>
#include <cmath>

using std::vector;

class Peaks
{
protected:
    static int pre;
    static int post;
	
public:
    /** General peak picking method for finding n local maxima in an array
     *  @param data input data
     *  @param peaks list of peak indexes
     *  @param width minimum distance between peaks
     */
    static int findPeaks(const vector<double> &data, vector<int> peaks, int width) {
	int peakCount = 0;
	int maxp = 0;
	int mid = 0;
	int end = data.size();
	while (mid < end) {
	    int i = mid - width;
	    if (i < 0)
		i = 0;
	    int stop = mid + width + 1;
	    if (stop > data.size())
		stop = data.size();
	    maxp = i;
	    for (i++; i < stop; i++)
		if (data[i] > data[maxp])
		    maxp = i;
	    if (maxp == mid) {
		int j;
		for (j = peakCount; j > 0; j--) {
		    if (data[maxp] <= data[peaks[j-1]])
			break;
		    else if (j < peaks.size())
			peaks[j] = peaks[j-1];
		}
		if (j != peaks.size())
		    peaks[j] = maxp;
		if (peakCount != peaks.size())
		    peakCount++;
	    }
	    mid++;
	}
	return peakCount;
    } // findPeaks()

    /** General peak picking method for finding local maxima in an array
     *  @param data input data
     *  @param width minimum distance between peaks
     *  @param threshold minimum value of peaks
     *  @return list of peak indexes
     */ 
    static vector<int> findPeaks(const vector<double> &data, int width,
				 double threshold) {
	return findPeaks(data, width, threshold, 0, false);
    } // findPeaks()
	
    /** General peak picking method for finding local maxima in an array
     *  @param data input data
     *  @param width minimum distance between peaks
     *  @param threshold minimum value of peaks
     *  @param decayRate how quickly previous peaks are forgotten
     *  @param isRelative minimum value of peaks is relative to local average
     *  @return list of peak indexes
     */
    static vector<int> findPeaks(const vector<double> &data, int width,
				 double threshold, double decayRate, bool isRelative) {
	vector<int> peaks;
	int maxp = 0;
	int mid = 0;
	int end = data.size();
	double av = data[0];
	while (mid < end) {
	    av = decayRate * av + (1 - decayRate) * data[mid];
	    if (av < data[mid])
		av = data[mid];
	    int i = mid - width;
	    if (i < 0)
		i = 0;
	    int stop = mid + width + 1;
	    if (stop > data.size())
		stop = data.size();
	    maxp = i;
	    for (i++; i < stop; i++)
		if (data[i] > data[maxp])
		    maxp = i;
	    if (maxp == mid) {
		if (overThreshold(data, maxp, width, threshold, isRelative,av)){
		    peaks.push_back(maxp);
		}
	    }
	    mid++;
	}
	return peaks;
    } // findPeaks()

    static double expDecayWithHold(double av, double decayRate,
				   const vector<double> &data, int start, int stop) {
	while (start < stop) {
	    av = decayRate * av + (1 - decayRate) * data[start];
	    if (av < data[start])
		av = data[start];
	    start++;
	}
	return av;
    } // expDecayWithHold()

    static bool overThreshold(const vector<double> &data, int index, int width,
				 double threshold, bool isRelative,
				 double av) {
	if (data[index] < av)
	    return false;
	if (isRelative) {
	    int iStart = index - pre * width;
	    if (iStart < 0)
		iStart = 0;
	    int iStop = index + post * width;
	    if (iStop > data.size())
		iStop = data.size();
	    double sum = 0;
	    int count = iStop - iStart;
	    while (iStart < iStop)
		sum += data[iStart++];
	    return (data[index] > sum / count + threshold);
	} else
	    return (data[index] > threshold);
    } // overThreshold()

    static void normalise(vector<double> &data) {
	double sx = 0;
	double sxx = 0;
	for (int i = 0; i < data.size(); i++) {
	    sx += data[i];
	    sxx += data[i] * data[i];
	}
	double mean = sx / data.size();
	double sd = sqrt((sxx - sx * mean) / data.size());
	if (sd == 0)
	    sd = 1;		// all data[i] == mean  -> 0; avoids div by 0
	for (int i = 0; i < data.size(); i++) {
	    data[i] = (data[i] - mean) / sd;
	}
    } // normalise()

    /** Uses an n-point linear regression to estimate the slope of data.
     *  @param data input data
     *  @param hop spacing of data points
     *  @param n length of linear regression
     *  @param slope output data
     */
    static void getSlope(const vector<double> &data, double hop, int n,
			 vector<double> &slope) {
	int i = 0, j = 0;
	double t;
	double sx = 0, sxx = 0, sy = 0, sxy = 0;
	for ( ; i < n; i++) {
	    t = i * hop;
	    sx += t;
	    sxx += t * t;
	    sy += data[i];
	    sxy += t * data[i];
	}
	double delta = n * sxx - sx * sx;
	for ( ; j < n / 2; j++)
	    slope[j] = (n * sxy - sx * sy) / delta;
	for ( ; j < data.size() - (n + 1) / 2; j++, i++) {
	    slope[j] = (n * sxy - sx * sy) / delta;
	    sy += data[i] - data[i - n];
	    sxy += hop * (n * data[i] - sy);
	}
	for ( ; j < data.size(); j++)
	    slope[j] = (n * sxy - sx * sy) / delta;
    } // getSlope()

    static double min(const vector<double> &arr) { return arr[imin(arr)]; }

    static double max(const vector<double> &arr) { return arr[imax(arr)]; }

    static int imin(const vector<double> &arr) {
	int i = 0;
	for (int j = 1; j < arr.size(); j++)
	    if (arr[j] < arr[i])
		i = j;
	return i;
    } // imin()

    static int imax(const vector<double> &arr) {
	int i = 0;
	for (int j = 1; j < arr.size(); j++)
	    if (arr[j] > arr[i])
		i = j;
	return i;
    } // imax()

}; // class Peaks

#endif
