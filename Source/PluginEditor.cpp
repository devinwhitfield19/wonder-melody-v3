#include "PluginEditor.h"
WonderMelodyGeneratorAudioProcessorEditor::WonderMelodyGeneratorAudioProcessorEditor(WonderMelodyGeneratorAudioProcessor& p):AudioProcessorEditor(&p),processor(p)
{
 setSize(960,700);
 title.setText("WONDER MELODY GENERATOR  /  V3",juce::dontSendNotification);
 title.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold)));
 title.setColour(juce::Label::textColourId,juce::Colour(0xffd6b0ff));addAndMakeVisible(title);
 const char* keys[]={"C Minor","C# Minor","D Minor","Eb Minor","E Minor","F Minor","F# Minor","G Minor","Ab Minor","A Minor","Bb Minor","B Minor"};
 for(int i=0;i<12;++i)keyBox.addItem(keys[i],i+1);keyBox.setSelectedId(processor.getRootNote()+1);
 keyBox.onChange=[this]{processor.setRootNote(keyBox.getSelectedId()-1);};addAndMakeVisible(keyBox);
 for(int i=1;i<=4;++i)densityBox.addItem(juce::String(i),i);densityBox.setSelectedId(processor.getDensity());
 densityBox.onChange=[this]{processor.setDensity(densityBox.getSelectedId());};addAndMakeVisible(densityBox);
 tempoSlider.setRange(60,180,1);tempoSlider.setValue(processor.getTempo());tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,24);
 tempoSlider.onValueChange=[this]{processor.setTempo((int)tempoSlider.getValue());};addAndMakeVisible(tempoSlider);
 swingSlider.setRange(0,0.6,0.01);swingSlider.setValue(processor.getSwing());swingSlider.onValueChange=[this]{processor.setSwing((float)swingSlider.getValue());};addAndMakeVisible(swingSlider);
 polySlider.setRange(4,32,1);polySlider.setValue(processor.getPolyphony());polySlider.onValueChange=[this]{processor.setPolyphony((int)polySlider.getValue());};addAndMakeVisible(polySlider);
 cutoffSlider.setRange(300,18000,1);cutoffSlider.setValue(processor.getFilterCutoff());cutoffSlider.onValueChange=[this]{processor.setFilterCutoff((float)cutoffSlider.getValue());};addAndMakeVisible(cutoffSlider);
 attackSlider.setRange(1,500,1);attackSlider.setValue(processor.getAttack());attackSlider.onValueChange=[this]{processor.setAttack((float)attackSlider.getValue());};addAndMakeVisible(attackSlider);
 releaseSlider.setRange(20,3000,1);releaseSlider.setValue(processor.getRelease());releaseSlider.onValueChange=[this]{processor.setRelease((float)releaseSlider.getValue());};addAndMakeVisible(releaseSlider);
 presetBox.addItem("Warm Dual Osc",1);presetBox.addItem("Bright Harmonics",2);presetBox.addItem("Soft Pad",3);presetBox.addItem("Pulse Lead",4);
 presetBox.setSelectedId(processor.getPreset()+1);presetBox.onChange=[this]{processor.setPreset(presetBox.getSelectedId()-1);};addAndMakeVisible(presetBox);
 generateButton.onClick=[this]{processor.generatePattern();status.setText("Generated melody, chords, bass and drums.",juce::dontSendNotification);repaint();};addAndMakeVisible(generateButton);
 chordButton.onClick=[this]{processor.generateChordBass();status.setText("Chord progression and bass rebuilt.",juce::dontSendNotification);repaint();};addAndMakeVisible(chordButton);
 drumsButton.onClick=[this]{processor.randomizeDrums();for(int i=0;i<16;++i)drumButtons[i].setToggleState(processor.getDrumStep(i),juce::dontSendNotification);};addAndMakeVisible(drumsButton);
 for(int i=0;i<16;++i){drumButtons[i].setButtonText(juce::String(i+1));drumButtons[i].setClickingTogglesState(true);
  drumButtons[i].setToggleState(processor.getDrumStep(i),juce::dontSendNotification);
  drumButtons[i].onClick=[this,i]{processor.setDrumStep(i,drumButtons[i].getToggleState());};addAndMakeVisible(drumButtons[i]);}
 promptEditor.setText("Create an emotional melodic trap idea in a minor key with a memorable hook, rich chord voicings, moving bass and syncopated drums.");addAndMakeVisible(promptEditor);
 endpointEditor.setText(processor.getAiEndpoint());addAndMakeVisible(endpointEditor);
 aiButton.onClick=[this]{processor.setAiEndpoint(endpointEditor.getText());processor.requestAiGeneration(promptEditor.getText());status.setText("AI request in progress...",juce::dontSendNotification);};addAndMakeVisible(aiButton);
 exportButton.onClick=[this]{exportMidi();};addAndMakeVisible(exportButton);
 status.setColour(juce::Label::textColourId,juce::Colour(0xff73e6ad));status.setText("Ready. MIDI playback follows host transport when playing.",juce::dontSendNotification);addAndMakeVisible(status);
 startTimerHz(5);
}
void WonderMelodyGeneratorAudioProcessorEditor::timerCallback()
{
 if(processor.consumePendingAiResult()){status.setText("AI pattern loaded. Edit the drum steps or export all four tracks.",juce::dontSendNotification);repaint();}
 else status.setText(processor.getAiStatus(),juce::dontSendNotification);
 for(int i=0;i<16;++i)drumButtons[i].setToggleState(processor.getDrumStep(i),juce::dontSendNotification);
}

void WonderMelodyGeneratorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080d17));
    g.setColour(juce::Colour(0xff111c2c));
    g.fillRoundedRectangle(16, 55, 928, 155, 12);
    g.setColour(juce::Colour(0xff273650));
    g.drawRoundedRectangle(16, 55, 928, 155, 12, 1);

    g.setColour(juce::Colour(0xffce5cff));
    g.setFont(14.0f);
    g.drawText("MELODY PIANO ROLL PREVIEW",
               juce::Rectangle<int>(28, 220, 400, 22),
               juce::Justification::left);

    juce::Rectangle<float> r(28, 246, 900, 88);
    g.setColour(juce::Colour(0xff0c1422));
    g.fillRoundedRectangle(r, 6);

    for (int i = 0; i <= 32; ++i)
    {
        g.setColour(juce::Colour(0xff26334a));
        g.drawVerticalLine(28 + i * 28, 246, 334);
    }

    const auto& notes = processor.getMelody();
    for (int i = 0; i < 32; ++i)
    {
        if (notes[(size_t)i] >= 0)
        {
            float y = 326 - (notes[(size_t)i] - 48) * 2.4f;
            y = juce::jlimit(248.0f, 322.0f, y);
            g.setColour(juce::Colour(0xffce5cff));
            g.fillRoundedRectangle(30 + i * 28, y, 20, 7, 2);
        }
    }

    g.setColour(juce::Colour(0xff94a3bd));
    g.setFont(12.0f);
    g.drawText("DRUM PATTERN — click steps to toggle",
               juce::Rectangle<int>(28, 345, 500, 20),
               juce::Justification::left);

    g.drawText("SYNTH: dual oscillators - low-pass filter - attack/release envelope - 4 presets",
               juce::Rectangle<int>(28, 425, 850, 20),
               juce::Justification::left);

    g.drawText("AI uses a local Python bridge; prompts are sent to the configured model when requested.",
               juce::Rectangle<int>(28, 540, 850, 20),
               juce::Justification::left);
}

void WonderMelodyGeneratorAudioProcessorEditor::resized()
{
 title.setBounds(22,10,800,36);
 keyBox.setBounds(28,72,130,28);tempoSlider.setBounds(174,72,130,28);densityBox.setBounds(320,72,70,28);
 swingSlider.setBounds(405,72,110,28);polySlider.setBounds(530,72,110,28);presetBox.setBounds(655,72,180,28);
 cutoffSlider.setBounds(28,145,180,28);attackSlider.setBounds(230,145,180,28);releaseSlider.setBounds(430,145,180,28);
 generateButton.setBounds(640,132,130,38);chordButton.setBounds(780,132,145,38);
 for(int i=0;i<16;++i)drumButtons[i].setBounds(28+i*56,370,50,34);
 drumsButton.setBounds(28,390,160,30);exportButton.setBounds(205,390,200,30);
 promptEditor.setBounds(28,465,560,52);endpointEditor.setBounds(610,465,315,28);aiButton.setBounds(610,500,315,34);
 status.setBounds(28,600,900,34);
}

void WonderMelodyGeneratorAudioProcessorEditor::exportMidi()
{
    auto midiFile = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory).getChildFile("WonderMelody.mid");

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export four-track MIDI",
        midiFile,
        "*.mid");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            if (!file.hasFileExtension("mid"))
                file = file.withFileExtension("mid");

            processor.exportAllMidi(file);
            status.setText(
                "Exported four MIDI tracks: Melody, Chords, Bass, Drums.",
                juce::dontSendNotification);
        });
}
