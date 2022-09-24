/*
  ==============================================================================

    synthsound.h
    Created: 12 Sep 2022 9:37:18pm
    Author:  Tharindu Damruwan

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int midiNoteNumber) override { return true; }
    bool appliesToChannel (int midiChannel) override { return true; }
    
};
