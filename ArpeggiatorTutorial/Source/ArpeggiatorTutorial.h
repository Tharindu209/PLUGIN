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

 name:             ArpeggiatorTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Arpeggiator audio plugin.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        Arpeggiator

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class Arpeggiator  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Arpeggiator()
        : AudioProcessor (BusesProperties()) // add no audio buses at all
    {
        addParameter (speed = new juce::AudioParameterFloat ("speed", "Arpeggiator Speed", 0.0, 1.0, 0.5));
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int) override
    {
        notes.clear();                          // [1]
        currentNote = 0;                        // [2]
        lastNoteValue = -1;                     // [3]
        time = 0;                               // [4]
        rate = static_cast<float> (sampleRate); // [5]
    }

    void releaseResources() override {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        // the audio buffer in a midi effect will have zero channels!
        jassert (buffer.getNumChannels() == 0);                                                         // [6]

        // however we use the buffer to get timing information
        auto numSamples = buffer.getNumSamples();                                                       // [7]

        // get note duration
        auto noteDuration = static_cast<int> (std::ceil (rate * 0.25f * (0.1f + (1.0f - (*speed)))));   // [8]

        for (const auto metadata : midi)                                                                // [9]
        {
            const auto msg = metadata.getMessage();

            if      (msg.isNoteOn())  notes.add (msg.getNoteNumber());
            else if (msg.isNoteOff()) notes.removeValue (msg.getNoteNumber());
        }

        midi.clear();                                                                                   // [10]

        if ((time + numSamples) >= noteDuration)                                                        // [11]
        {
            auto offset = juce::jmax (0, juce::jmin ((int) (noteDuration - time), numSamples - 1));     // [12]

            if (lastNoteValue > 0)                                                                      // [13]
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, lastNoteValue), offset);
                lastNoteValue = -1;
            }

            if (notes.size() > 0)                                                                       // [14]
            {
                currentNote = (currentNote + 1) % notes.size();
                lastNoteValue = notes[currentNote];
                midi.addEvent (juce::MidiMessage::noteOn  (1, lastNoteValue, (juce::uint8) 127), offset);
            }

        }

        time = (time + numSamples) % noteDuration;                                                      // [15]
    }

    //==============================================================================
    bool isMidiEffect() const override                           { return true; }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override          { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                              { return true; }

    //==============================================================================
    const juce::String getName() const override                  { return "Arpeggiator"; }

    bool acceptsMidi() const override                            { return true; }
    bool producesMidi() const override                           { return true; }
    double getTailLengthSeconds() const override                 { return 0; }

    //==============================================================================
    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream (destData, true).writeFloat (*speed);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        speed->setValueNotifyingHost (juce::MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat());
    }

private:
    //==============================================================================
    juce::AudioParameterFloat* speed;
    int currentNote, lastNoteValue;
    int time;
    float rate;
    juce::SortedSet<int> notes;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arpeggiator)
};
