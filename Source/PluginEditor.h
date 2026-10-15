#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class WonderMelodyGeneratorAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
 explicit WonderMelodyGeneratorAudioProcessorEditor(WonderMelodyGeneratorAudioProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 void timerCallback() override;
 void exportMidi();
 WonderMelodyGeneratorAudioProcessor& processor;
 juce::ComboBox keyBox,densityBox,presetBox;
 juce::Slider tempoSlider,swingSlider,polySlider,cutoffSlider,attackSlider,releaseSlider;
 juce::TextButton generateButton{"GENERATE"},chordButton{"CHORDS + BASS"},drumsButton{"RANDOMIZE DRUMS"},exportButton{"EXPORT 4-TRACK MIDI"},aiButton{"GENERATE WITH AI"};
 juce::TextButton drumButtons[16];
 juce::TextEditor promptEditor,endpointEditor;
 juce::Label title,status;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WonderMelodyGeneratorAudioProcessorEditor)
};
