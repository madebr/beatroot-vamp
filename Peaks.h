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
    static int findPeaks(const vector<double> &data, vector<int> peaks, int width);

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
				 double threshold, double decayRate, bool isRelative);

    static double expDecayWithHold(double av, double decayRate,
				   const vector<double> &data, int start, int stop);

    static bool overThreshold(const vector<double> &data, int index, int width,
                              double threshold, bool isRelative,
                              double av);

    static void normalise(vector<double> &data);

    /** Uses an n-point linear regression to estimate the slope of data.
     *  @param data input data
     *  @param hop spacing of data points
     *  @param n length of linear regression
     *  @param slope output data
     */
    static void getSlope(const vector<double> &data, double hop, int n,
			 vector<double> &slope);

    static double min(const vector<double> &arr) { return arr[imin(arr)]; }
    static double max(const vector<double> &arr) { return arr[imax(arr)]; }

    static int imin(const vector<double> &arr);
    static int imax(const vector<double> &arr);

}; // class Peaks

#endif
