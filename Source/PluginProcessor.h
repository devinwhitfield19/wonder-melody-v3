#pragma once
#include <JuceHeader.h>
#include <array>
#include <random>

class WonderMelodyGeneratorAudioProcessor : public juce::AudioProcessor
{
public:
    WonderMelodyGeneratorAudioProcessor();
    ~WonderMelodyGeneratorAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void generatePattern();
    void generateChordBass();
    void randomizeDrums();
    void setDrumStep(int step, bool enabled);
    bool getDrumStep(int step) const;
    void setRootNote(int n) { rootNote = juce::jlimit(0, 11, n); }
    void setTempo(int n) { tempo = juce::jlimit(60, 180, n); }
    void setDensity(int n) { density = juce::jlimit(1, 4, n); }
    void setSwing(float v) { swing = juce::jlimit(0.0f, 0.65f, v); }
    void setPolyphony(int n) { polyphony = juce::jlimit(4, 32, n); }
    int getRootNote() const { return rootNote; }
    int getTempo() const { return tempo; }
    int getDensity() const { return density; }
    float getSwing() const { return swing; }
    int getPolyphony() const { return polyphony; }
    const std::array<int, 32>& getMelody() const { return melody; }
    const std::array<std::array<int, 3>, 8>& getChordNotes() const { return chordNotes; }
    const std::array<int, 8>& getBassNotes() const { return bassNotes; }
    const std::array<bool, 16>& getDrums() const { return drumSteps; }
    bool getAiEnabled() const { return aiEnabled; }
    void setAiEnabled(bool v) { aiEnabled = v; }
    void setAiEndpoint(const juce::String& s) { aiEndpoint = s; }
    void setAiModel(const juce::String& s) { aiModel = s; }
    juce::String getAiEndpoint() const { return aiEndpoint; }
    juce::String getAiModel() const { return aiModel; }
    void requestAiGeneration(const juce::String& prompt);
    juce::String getAiStatus() const;
    bool consumePendingAiResult();
    void setFilterCutoff(float v) { filterCutoff.store(juce::jlimit(300.0f, 18000.0f, v)); }
    void setAttack(float v) { attackMs.store(juce::jlimit(1.0f, 500.0f, v)); }
    void setRelease(float v) { releaseMs.store(juce::jlimit(20.0f, 3000.0f, v)); }
    void setPreset(int v) { preset.store(juce::jlimit(0, 3, v)); }
    float getFilterCutoff() const { return filterCutoff.load(); }
    float getAttack() const { return attackMs.load(); }
    float getRelease() const { return releaseMs.load(); }
    int getPreset() const { return preset.load(); }
    void exportAllMidi(const juce::File& target);

private:
    struct Voice
    {
        double phase = 0.0, phase2 = 0.0;
        float env = 0.0f;
        int note = -1;
        int channel = 1;
        bool releasing = false;
    };
    static constexpr int voiceCount = 32;
    std::array<Voice, voiceCount> voices {};
    void noteOn(int note, int channel, float velocity);
    void noteOff(int note, int channel);
    void renderSynth(juce::AudioBuffer<float>& buffer, int samples);
    void applyAiJson(const juce::var& data);
    void scheduleGeneratedMidi(juce::MidiBuffer& midi, int numSamples,
                               const juce::AudioPlayHead::CurrentPositionInfo& pos,
                               bool havePosition);

    std::array<int, 32> melody {};
    std::array<std::array<int, 3>, 8> chordNotes {};
    std::array<int, 8> bassNotes {};
    std::array<bool, 16> drumSteps {};
    std::mt19937 rng { std::random_device{}() };
    mutable juce::CriticalSection aiLock;
    juce::var pendingAiData;
    juce::String aiStatus { "Procedural engine ready" };
    std::atomic<float> filterCutoff { 12000.0f }, attackMs { 8.0f }, releaseMs { 350.0f };
    std::atomic<int> preset { 0 };
    juce::dsp::StateVariableTPTFilter<float> filterL, filterR;
    float filterStateL = 0.0f, filterStateR = 0.0f;
    double lastGeneratedPpq = -1.0;
    int lastGeneratedStep = -1;
    std::array<int, 4> lastNotes {{-1,-1,-1,-1}};
    double sampleRate = 44100.0;
    int rootNote = 0, tempo = 140, density = 2, polyphony = 16;
    float swing = 0.12f;
    bool aiEnabled = false;
    juce::String aiEndpoint { "http://127.0.0.1:8765/generate" };
    juce::String aiModel { "gpt-4.1-mini" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WonderMelodyGeneratorAudioProcessor)
};
