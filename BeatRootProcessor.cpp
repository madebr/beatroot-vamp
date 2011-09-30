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

double
BeatRootProcessor::silenceThreshold = 0.0004;

double
BeatRootProcessor::rangeThreshold = 10;

int
BeatRootProcessor::normaliseMode = 2;

//int
//BeatRootProcessor::energyOversampleFactor = 2;

