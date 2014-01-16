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
    Plugin(inputSampleRate),
    m_firstFrame(true)
{
    m_processor = new BeatRootProcessor(inputSampleRate, AgentParameters());
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

    ParameterDescriptor desc;

    desc.identifier = "preMarginFactor";
    desc.name = "Pre-Margin Factor";
    desc.description = "The maximum amount by which a beat can be earlier than the predicted beat time, expressed as a fraction of the beat period.";
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.defaultValue = AgentParameters::DEFAULT_PRE_MARGIN_FACTOR;
    desc.isQuantized = false;
    list.push_back(desc);
    
    desc.identifier = "postMarginFactor";
    desc.name = "Post-Margin Factor";
    desc.description = "The maximum amount by which a beat can be later than the predicted beat time, expressed as a fraction of the beat period.";
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.defaultValue = AgentParameters::DEFAULT_POST_MARGIN_FACTOR;
    desc.isQuantized = false;
    list.push_back(desc);
    
    desc.identifier = "maxChange";
    desc.name = "Maximum Change";
    desc.description = "The maximum allowed deviation from the initial tempo, expressed as a fraction of the initial beat period.";
    desc.minValue = 0;
    desc.maxValue = 1;
    desc.defaultValue = AgentParameters::DEFAULT_MAX_CHANGE;
    desc.isQuantized = false;
    list.push_back(desc);
    
    desc.identifier = "expiryTime";
    desc.name = "Expiry Time";
    desc.description = "The default value of expiryTime, which is the time (in seconds) after which an Agent that has no Event matching its beat predictions will be destroyed.";
    desc.minValue = 2;
    desc.maxValue = 120;
    desc.defaultValue = AgentParameters::DEFAULT_EXPIRY_TIME;
    desc.isQuantized = false;
    list.push_back(desc);

    // Simon says...

    // These are the parameters that should be exposed (Agent.cpp):

    // If Pop, both margins should be lower (0.1).  If classical
    // music, post margin can be increased
    //
    // double Agent::POST_MARGIN_FACTOR = 0.3;
    // double Agent::PRE_MARGIN_FACTOR = 0.15;
    //
    // Max Change tells us how much tempo can change - so for
    // classical we should make it higher
    // 
    // double Agent::MAX_CHANGE = 0.2;
    // 
    // The EXPIRY TIME default should be defaulted to 100 (usual cause
    // of agents dying....)  it should also be exposed in order to
    // troubleshoot eventual problems in songs with big silences in
    // the beggining/end.
    // 
    // const double Agent::DEFAULT_EXPIRY_TIME = 10.0;

    return list;
}

float
BeatRootVampPlugin::getParameter(string identifier) const
{
    if (identifier == "preMarginFactor") {
        return m_parameters.preMarginFactor;
    } else if (identifier == "postMarginFactor") {
        return m_parameters.postMarginFactor;
    } else if (identifier == "maxChange") {
        return m_parameters.maxChange;
    } else if (identifier == "expiryTime") {
        return m_parameters.expiryTime;
    }
    
    return 0;
}

void
BeatRootVampPlugin::setParameter(string identifier, float value) 
{
    if (identifier == "preMarginFactor") {
        m_parameters.preMarginFactor = value;
    } else if (identifier == "postMarginFactor") {
        m_parameters.postMarginFactor = value;
    } else if (identifier == "maxChange") {
        m_parameters.maxChange = value;
    } else if (identifier == "expiryTime") {
        m_parameters.expiryTime = value;
    }
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

    d.identifier = "unfilled";
    d.name = "Un-interpolated beats";
    d.description = "Locations of detected beats, before agent interpolation occurs";
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

    // Delete the processor that was created with default parameters
    // and used to determine the expected step and block size; replace
    // with one using the actual parameters we have
    delete m_processor;
    m_processor = new BeatRootProcessor(m_inputSampleRate, m_parameters);

    return true;
}

void
BeatRootVampPlugin::reset()
{
    m_processor->reset();
    m_firstFrame = true;
    m_origin = Vamp::RealTime::zeroTime;
}

BeatRootVampPlugin::FeatureSet
BeatRootVampPlugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    if (m_firstFrame) {
        m_origin = timestamp;
        m_firstFrame = false;
    }

    m_processor->processFrame(inputBuffers);
    return FeatureSet();
}

BeatRootVampPlugin::FeatureSet
BeatRootVampPlugin::getRemainingFeatures()
{
    EventList unfilled;
    EventList el = m_processor->beatTrack(&unfilled);

    Feature f;
    f.hasTimestamp = true;
    f.hasDuration = false;
    f.label = "";
    f.values.clear();

    FeatureSet fs;

    for (EventList::const_iterator i = el.begin(); i != el.end(); ++i) {
        f.timestamp = m_origin + Vamp::RealTime::fromSeconds(i->time);
        fs[0].push_back(f);
    }

    for (EventList::const_iterator i = unfilled.begin(); 
         i != unfilled.end(); ++i) {
        f.timestamp = m_origin + Vamp::RealTime::fromSeconds(i->time);
        fs[1].push_back(f);
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

