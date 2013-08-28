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

#include "BeatRootVampPlugin.h"
#include "BeatRootProcessor.h"

#include "Event.h"

#include <vamp-sdk/RealTime.h>
#include <vamp-sdk/PluginAdapter.h>

BeatRootVampPlugin::BeatRootVampPlugin(float inputSampleRate) :
    Plugin(inputSampleRate)
{
    m_processor = new BeatRootProcessor(inputSampleRate);
}

BeatRootVampPlugin::~BeatRootVampPlugin()
{
    delete m_processor;
}

string
BeatRootVampPlugin::getIdentifier() const
{
    return "beatroot";
}

string
BeatRootVampPlugin::getName() const
{
    return "BeatRoot Beat Tracker";
}

string
BeatRootVampPlugin::getDescription() const
{
    return "Identify beat locations in music";
}

string
BeatRootVampPlugin::getMaker() const
{
    return "Simon Dixon (plugin by Chris Cannam)";
}

int
BeatRootVampPlugin::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string
BeatRootVampPlugin::getCopyright() const
{
    return "GPL";
}

BeatRootVampPlugin::InputDomain
BeatRootVampPlugin::getInputDomain() const
{
    return FrequencyDomain;
}

size_t
BeatRootVampPlugin::getPreferredBlockSize() const
{
    return m_processor->getFFTSize();
}

size_t 
BeatRootVampPlugin::getPreferredStepSize() const
{
    return m_processor->getHopSize();
}

size_t
BeatRootVampPlugin::getMinChannelCount() const
{
    return 1;
}

size_t
BeatRootVampPlugin::getMaxChannelCount() const
{
    return 1;
}

BeatRootVampPlugin::ParameterList
BeatRootVampPlugin::getParameterDescriptors() const
{
    ParameterList list;
    return list;
}

float
BeatRootVampPlugin::getParameter(string identifier) const
{
    return 0;
}

void
BeatRootVampPlugin::setParameter(string identifier, float value) 
{
}

BeatRootVampPlugin::ProgramList
BeatRootVampPlugin::getPrograms() const
{
    ProgramList list;
    return list;
}

string
BeatRootVampPlugin::getCurrentProgram() const
{
    return ""; // no programs
}

void
BeatRootVampPlugin::selectProgram(string name)
{
}

BeatRootVampPlugin::OutputList
BeatRootVampPlugin::getOutputDescriptors() const
{
    OutputList list;

    // See OutputDescriptor documentation for the possibilities here.
    // Every plugin must have at least one output.

    OutputDescriptor d;
    d.identifier = "beats";
    d.name = "Beats";
    d.description = "Estimated beat locations";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 0;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = m_inputSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    return list;
}

bool
BeatRootVampPlugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) {
	std::cerr << "BeatRootVampPlugin::initialise: Unsupported number ("
		  << channels << ") of channels" << std::endl;
	return false;
    }

    if (stepSize != getPreferredStepSize()) {
	std::cerr << "BeatRootVampPlugin::initialise: Unsupported step size "
		  << "for sample rate (" << stepSize << ", required step is "
		  << getPreferredStepSize() << " for rate " << m_inputSampleRate
		  << ")" << std::endl;
	return false;
    }

    if (blockSize != getPreferredBlockSize()) {
	std::cerr << "BeatRootVampPlugin::initialise: Unsupported block size "
		  << "for sample rate (" << blockSize << ", required size is "
		  << getPreferredBlockSize() << " for rate " << m_inputSampleRate
		  << ")" << std::endl;
	return false;
    }

    m_processor->reset();

    return true;
}

void
BeatRootVampPlugin::reset()
{
    m_processor->reset();
}

BeatRootVampPlugin::FeatureSet
BeatRootVampPlugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    m_processor->processFrame(inputBuffers);
    return FeatureSet();
}

BeatRootVampPlugin::FeatureSet
BeatRootVampPlugin::getRemainingFeatures()
{
    EventList el = m_processor->beatTrack();

    Feature f;
    f.hasTimestamp = true;
    f.hasDuration = false;
    f.label = "";
    f.values.clear();

    FeatureSet fs;

    for (EventList::const_iterator i = el.begin(); i != el.end(); ++i) {
        f.timestamp = Vamp::RealTime::fromSeconds(i->time);
        fs[0].push_back(f);
    }

    return fs;
}


static Vamp::PluginAdapter<BeatRootVampPlugin> brAdapter;

const VampPluginDescriptor *vampGetPluginDescriptor(unsigned int version,
                                                    unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case  0: return brAdapter.getDescriptor();
    default: return 0;
    }
}

