/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             NoiseGateTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Noise gate audio plugin.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        NoiseGate

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class NoiseGate  : public juce::AudioProcessor
{
public:
    //==============================================================================
//! [constructor]
    NoiseGate()
        : AudioProcessor (BusesProperties().withInput  ("Input",     juce::AudioChannelSet::stereo())           // [1]
                                           .withOutput ("Output",    juce::AudioChannelSet::stereo())
                                           .withInput  ("Sidechain", juce::AudioChannelSet::stereo()))
    {
        addParameter (threshold = new juce::AudioParameterFloat ("threshold", "Threshold", 0.0f, 1.0f, 0.5f));  // [2]
        addParameter (alpha     = new juce::AudioParameterFloat ("alpha",     "Alpha",     0.0f, 1.0f, 0.8f));
    }
//! [constructor]

    //==============================================================================
//! [isBusesLayoutSupported]
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // the sidechain can take any layout, the main bus needs to be the same on the input and output
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
                 && ! layouts.getMainInputChannelSet().isDisabled();
    }
//! [isBusesLayoutSupported]

    //==============================================================================
//! [prepareToPlay]
    void prepareToPlay (double, int) override
    {
        lowPassCoeff = 0.0f;    // [3]
        sampleCountDown = 0;    // [4]
    }
//! [prepareToPlay]

    void releaseResources() override          {}

//! [processBlock]
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto mainInputOutput = getBusBuffer (buffer, true, 0);                                  // [5]
        auto sideChainInput  = getBusBuffer (buffer, true, 1);

        auto alphaCopy = alpha->get();                                                          // [6]
        auto thresholdCopy = threshold->get();

        for (auto j = 0; j < buffer.getNumSamples(); ++j)                                       // [7]
        {
            auto mixedSamples = 0.0f;

            for (auto i = 0; i < sideChainInput.getNumChannels(); ++i)                          // [8]
                mixedSamples += sideChainInput.getReadPointer (i) [j];

            mixedSamples /= static_cast<float> (sideChainInput.getNumChannels());
            lowPassCoeff = (alphaCopy * lowPassCoeff) + ((1.0f - alphaCopy) * mixedSamples);    // [9]

            if (lowPassCoeff >= thresholdCopy)                                                  // [10]
                sampleCountDown = (int) getSampleRate();

            // very in-effective way of doing this
            for (auto i = 0; i < mainInputOutput.getNumChannels(); ++i)                         // [11]
                *mainInputOutput.getWritePointer (i, j) = sampleCountDown > 0 ? *mainInputOutput.getReadPointer (i, j)
                                                                              : 0.0f;

            if (sampleCountDown > 0)                                                            // [12]
                --sampleCountDown;
        }
    }
//! [processBlock]

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override            { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                                { return true; }
    const juce::String getName() const override                    { return "Noise Gate"; }
    bool acceptsMidi() const override                              { return false; }
    bool producesMidi() const override                             { return false; }
    double getTailLengthSeconds() const override                   { return 0.0; }
    int getNumPrograms() override                                  { return 1; }
    int getCurrentProgram() override                               { return 0; }
    void setCurrentProgram (int) override                          {}
    const juce::String getProgramName (int) override               { return {}; }
    void changeProgramName (int, const juce::String&) override     {}

    bool isVST2() const noexcept                                   { return (wrapperType == wrapperType_VST); }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream stream (destData, true);

        stream.writeFloat (*threshold);
        stream.writeFloat (*alpha);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

        threshold->setValueNotifyingHost (stream.readFloat());
        alpha->setValueNotifyingHost (stream.readFloat());
    }

//! [members]
private:
    //==============================================================================
    juce::AudioParameterFloat* threshold;
    juce::AudioParameterFloat* alpha;
    int sampleCountDown;

    float lowPassCoeff;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseGate)
};
//! [members]
