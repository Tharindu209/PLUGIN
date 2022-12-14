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

 name:             SurroundTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Surround audio plugin.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        SurroundProcessor

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class ChannelClickListener
{
public:
    virtual ~ChannelClickListener() {}
    virtual void channelButtonClicked (int channelIndex) = 0;
    virtual bool isChannelActive (int channelIndex) = 0;
};

class SurroundEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    SurroundEditor (juce::AudioProcessor& parent)
        : AudioProcessorEditor (parent),
          currentChannelLayout (juce::AudioChannelSet::disabled()),
          noChannelsLabel ("noChannelsLabel", "Input disabled"),
          layoutTitle ("LayoutTitleLabel", getLayoutName())
    {
        layoutTitle.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (layoutTitle);
        addAndMakeVisible (noChannelsLabel);

        setSize (600, 100);

        lastSuspended = ! getAudioProcessor()->isSuspended();
        timerCallback();
        startTimer (500);
    }

    void resized() override
    {
        auto r = getLocalBounds();

        layoutTitle.setBounds (r.removeFromBottom (16));

        noChannelsLabel.setBounds (r);

        if (channelButtons.size() > 0)
        {
            auto buttonWidth = r.getWidth() / channelButtons.size();
            for (auto channelButton : channelButtons)
                channelButton->setBounds (r.removeFromLeft (buttonWidth));
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void updateButton (juce::Button* btn)
    {
        if (auto* textButton = dynamic_cast<juce::TextButton*> (btn))
        {
            auto channelIndex = channelButtons.indexOf (textButton);

            if (auto* listener = dynamic_cast<ChannelClickListener*> (getAudioProcessor()))
                listener->channelButtonClicked (channelIndex);
        }
    }

    void updateGUI()
    {
        const auto& channelSet = getAudioProcessor()->getChannelLayoutOfBus (false, 0);

        if (channelSet != currentChannelLayout)
        {
            currentChannelLayout = channelSet;

            layoutTitle.setText (currentChannelLayout.getDescription(), juce::NotificationType::dontSendNotification);
            channelButtons.clear();
            activeChannels.resize (currentChannelLayout.size());

            if (currentChannelLayout == juce::AudioChannelSet::disabled())
            {
                noChannelsLabel.setVisible (true);
            }
            else
            {
                auto numChannels = currentChannelLayout.size();

                for (auto i = 0; i < numChannels; ++i)
                {
                    auto channelName =
                    juce::AudioChannelSet::getAbbreviatedChannelTypeName (currentChannelLayout.getTypeOfChannel (i));

                    juce::TextButton* newButton;
                    channelButtons.add (newButton = new juce::TextButton (channelName, channelName));

                    newButton->onClick = [this, newButton] { updateButton (newButton); };
                    addAndMakeVisible (newButton);
                }

                noChannelsLabel.setVisible (false);
                resized();
            }

            if (auto* listener = dynamic_cast<ChannelClickListener*> (getAudioProcessor()))
            {
                auto   activeColour = getLookAndFeel().findColour (juce::Slider::thumbColourId);
                auto inactiveColour = getLookAndFeel().findColour (juce::Slider::trackColourId);

                for (auto i = 0; i < activeChannels.size(); ++i)
                {
                    auto isActive = listener->isChannelActive (i);
                    activeChannels.getReference (i) = isActive;
                    channelButtons[i]->setColour (juce::TextButton::buttonColourId, isActive ? activeColour : inactiveColour);
                    channelButtons[i]->repaint();
                }
            }
        }
    }

private:
    juce::String getLayoutName() const
    {
        if (auto* p = getAudioProcessor())
            return p->getChannelLayoutOfBus (false, 0).getDescription();

        return "Unknown";
    }

    void timerCallback() override
    {
        if (getAudioProcessor()->isSuspended() != lastSuspended)
        {
            lastSuspended = getAudioProcessor()->isSuspended();
            updateGUI();
        }

        if (! lastSuspended)
        {
            if (auto* listener = dynamic_cast<ChannelClickListener*> (getAudioProcessor()))
            {
                auto   activeColour = getLookAndFeel().findColour (juce::Slider::thumbColourId);
                auto inactiveColour = getLookAndFeel().findColour (juce::Slider::trackColourId);

                for (auto i = 0; i < activeChannels.size(); ++i)
                {
                    auto isActive = listener->isChannelActive (i);
                    if (activeChannels.getReference (i) != isActive)
                    {
                        activeChannels.getReference (i) = isActive;
                        channelButtons[i]->setColour (juce::TextButton::buttonColourId, isActive ? activeColour : inactiveColour);
                        channelButtons[i]->repaint();
                    }
                }
            }
        }
    }

    juce::AudioChannelSet currentChannelLayout;
    juce::Label noChannelsLabel, layoutTitle;
    juce::OwnedArray<juce::TextButton> channelButtons;
    juce::Array<bool> activeChannels;

    bool lastSuspended;
};

//==============================================================================
class SurroundProcessor  : public juce::AudioProcessor,
                           private ChannelClickListener,
                           private juce::AsyncUpdater
{
public:
//! [constructor]
    SurroundProcessor()
        : AudioProcessor(BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo())
                                          .withOutput ("Output", juce::AudioChannelSet::stereo()))
    {}
//! [constructor]

    //==============================================================================
//! [prepareToPlay]
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        channelClicked = 0;                                             // [4]
        sampleOffset = static_cast<int> (std::ceil (sampleRate));       // [5]

        auto numChannels = getChannelCountOfBus (true, 0);              // [6]
        channelActive.resize (numChannels);
        alphaCoeffs.resize (numChannels);
        reset();                                                        // [7]

        triggerAsyncUpdate();                                           // [8]

        juce::ignoreUnused (samplesPerBlock);
    }
//! [prepareToPlay]

//! [releaseResources]
    void releaseResources() override { reset(); }
//! [releaseResources]

//! [processBlock top]
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)                   // [9]
        {
            auto& channelTime = channelActive.getReference (ch);
            auto& alpha = alphaCoeffs.getReference (ch);

            for (auto j = 0; j < buffer.getNumSamples(); ++j)                   // [10]
            {
                auto sample = buffer.getReadPointer (ch)[j];
                alpha = (0.8f * alpha) + (0.2f * sample);

                if (std::abs (alpha) >= 0.1f)                                   // [11]
                    channelTime = static_cast<int> (getSampleRate() / 2.0);
            }

            channelTime = juce::jmax (0, channelTime - buffer.getNumSamples()); // [12]
        }
//! [processBlock top]

//! [processBlock bottom]
        auto fillSamples = juce::jmin (static_cast<int> (std::ceil (getSampleRate())) - sampleOffset,
                                       buffer.getNumSamples());                 // [13]

        if (juce::isPositiveAndBelow (channelClicked, buffer.getNumChannels())) // [14]
        {
            auto* channelBuffer = buffer.getWritePointer (channelClicked);
            auto freq = (float) (440.0 / getSampleRate());

            for (auto i = 0; i < fillSamples; ++i)                              // [15]
                channelBuffer[i] += std::sin (2.0f * juce::MathConstants<float>::pi * freq * static_cast<float> (sampleOffset++));
        }
    }
//! [processBlock bottom]

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return new SurroundEditor (*this); }
    bool hasEditor() const override                     { return true; }

    //==============================================================================
//! [isBusesLayoutSupported]
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return ((! layouts.getMainInputChannelSet() .isDiscreteLayout())                // [1]
             && (! layouts.getMainOutputChannelSet().isDiscreteLayout())
             && (layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()) // [2]
             && (! layouts.getMainInputChannelSet().isDisabled()));                     // [3]
    }
//! [isBusesLayoutSupported]

//! [reset]
    void reset() override
    {
        for (auto& channel : channelActive)
            channel = 0;
    }
//! [reset]

    //==============================================================================
    const juce::String getName() const override                  { return "Surround"; }
    bool acceptsMidi() const override                            { return false; }
    bool producesMidi() const override                           { return false; }
    double getTailLengthSeconds() const override                 { return 0; }

    //==============================================================================
    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override   {}

//! [handleAsyncUpdate]
    void handleAsyncUpdate() override
    {
        if (auto* editor = getActiveEditor())
            if (auto* surroundEditor = dynamic_cast<SurroundEditor*> (editor))
                surroundEditor->updateGUI();
    }
//! [handleAsyncUpdate]

private:
//! [channelButtonClicked]
    void channelButtonClicked (int channelIndex) override
    {
        channelClicked = channelIndex;
        sampleOffset = 0;
    }
//! [channelButtonClicked]

//! [isChannelActive]
    bool isChannelActive (int channelIndex) override
    {
        return channelActive[channelIndex] > 0;
    }
//! [isChannelActive]

//! [members]
    juce::Array<int> channelActive;
    juce::Array<float> alphaCoeffs;
    int channelClicked;
    int sampleOffset;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SurroundProcessor)
};
//! [members]
