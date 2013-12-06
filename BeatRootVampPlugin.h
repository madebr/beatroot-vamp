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

#ifndef _BEATROOT_VAMP_PLUGIN_H_
#define _BEATROOT_VAMP_PLUGIN_H_

#include "Agent.h"

#include <vamp-sdk/Plugin.h>

using std::string;

class BeatRootProcessor;

class BeatRootVampPlugin : public Vamp::Plugin
{
public:
    BeatRootVampPlugin(float inputSampleRate);
    virtual ~BeatRootVampPlugin();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    BeatRootProcessor *m_processor;
    AgentParameters m_parameters;
    Vamp::RealTime m_origin;
    bool m_firstFrame;
};



#endif
