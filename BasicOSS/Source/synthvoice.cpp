/*
  ==============================================================================

    synthvoice.cpp
    Created: 12 Sep 2022 9:37:54pm
    Author:  Tharindu Damruwan

  ==============================================================================
*/

#include "synthvoice.h"

bool SynthVoice::canPlaySound (juce::SynthesiserSound *sound) {
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) {
    
}

void SynthVoice::stopNote (float velocity, bool allowTailOff) {
    
}
void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue) {
    
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue){
    
}

void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) {
    
}
