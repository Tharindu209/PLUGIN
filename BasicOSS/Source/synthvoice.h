/*
  ==============================================================================

    synthvoice.h
    Created: 12 Sep 2022 9:37:54pm
    Author:  Tharindu Damruwan

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "synthsound.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound *sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void prepare
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    
private:
    juce::dsp::Oscillator<float> osc{ [](float x) { return std::sin(x);}};
    juce::dsp::Gain<float> gain;
};
