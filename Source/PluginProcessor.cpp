#include "PluginProcessor.h"
#include "PluginEditor.h"

WonderMelodyGeneratorAudioProcessor::WonderMelodyGeneratorAudioProcessor()
 : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    generatePattern();
}
void WonderMelodyGeneratorAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    filterL.reset(); filterR.reset();
    filterL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterL.prepare({ sr, 512, 1 }); filterR.prepare({ sr, 512, 1 });
    filterL.setCutoffFrequency(filterCutoff.load());
    filterR.setCutoffFrequency(filterCutoff.load());
    lastGeneratedPpq = -1.0; lastGeneratedStep = -1;
}
bool WonderMelodyGeneratorAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}
void WonderMelodyGeneratorAudioProcessor::noteOn(int note, int channel, float velocity)
{
    int count=0; for (auto& v:voices) if(v.note>=0) ++count;
    if(count>=polyphony) {
        auto it=std::min_element(voices.begin(),voices.end(),[](const Voice&a,const Voice&b){return a.env<b.env;});
        it->note=-1;
    }
    auto it=std::find_if(voices.begin(),voices.end(),[](const Voice&v){return v.note<0;});
    if(it==voices.end()) it=voices.begin();
    *it=Voice{}; it->note=note; it->channel=channel; it->env=0.0f;
    it->releasing=false;
}
void WonderMelodyGeneratorAudioProcessor::noteOff(int note, int channel)
{
    for(auto& v:voices) if(v.note==note && v.channel==channel) v.releasing=true;
}
void WonderMelodyGeneratorAudioProcessor::renderSynth(juce::AudioBuffer<float>& b, int n)
{
    const float attack = juce::jmax(0.001f, attackMs.load()) * 0.001f;
    const float release = juce::jmax(0.02f, releaseMs.load()) * 0.001f;
    const float attackStep = 1.0f / (float)(sampleRate * attack);
    const float releaseCoef = std::exp(-1.0f / (float)(sampleRate * release));
    filterL.setCutoffFrequency(filterCutoff.load()); filterR.setCutoffFrequency(filterCutoff.load());
    for(int i=0;i<n;++i) {
        float mix=0.0f;
        for(auto& v:voices) {
            if(v.note<0) continue;
            const double hz=juce::MidiMessage::getMidiNoteInHertz(v.note);
            v.phase += juce::MathConstants<double>::twoPi*hz/sampleRate;
            v.phase2 += juce::MathConstants<double>::twoPi*hz*1.004/sampleRate;
            if(v.phase>juce::MathConstants<double>::twoPi) v.phase-=juce::MathConstants<double>::twoPi;
            if(v.phase2>juce::MathConstants<double>::twoPi) v.phase2-=juce::MathConstants<double>::twoPi;
            if(v.releasing) v.env*=releaseCoef;
            else v.env=juce::jmin(1.0f,v.env+attackStep);
            if(v.env<0.0002f) {v.note=-1;continue;}
            const int presetNow=preset.load();
            float wave=0.0f;
            if(presetNow==0) wave=(float)(std::sin(v.phase)*0.72+std::sin(v.phase2)*0.28);
            else if(presetNow==1) wave=(float)(std::sin(v.phase)*0.55+std::sin(v.phase*2.0)*0.30+std::sin(v.phase*3.0)*0.15);
            else if(presetNow==2) wave=(float)(std::sin(v.phase)*0.85+std::sin(v.phase2*0.5)*0.15);
            else wave=(float)(std::sin(v.phase)>0.0 ? 0.7 : -0.7); // softened pulse-style lead
            mix += std::tanh(wave*v.env*0.12f);
        }
        float l=mix, r=mix;
        l=filterL.processSample(0,l); r=filterR.processSample(0,r);
        if(b.getNumChannels()>0) b.addSample(0,i,l);
        if(b.getNumChannels()>1) b.addSample(1,i,r);
    }
}
void WonderMelodyGeneratorAudioProcessor::scheduleGeneratedMidi(juce::MidiBuffer& midi, int numSamples,
    const juce::AudioPlayHead::CurrentPositionInfo& pos, bool havePosition)
{
    if(!havePosition || !pos.isPlaying || pos.bpm<=1.0) return;
    const double bpm=pos.bpm;
    const double ppqStart=pos.ppqPosition;
    const double ppqPerSample=bpm/(60.0*sampleRate);
    const double ppqEnd=ppqStart+numSamples*ppqPerSample;
    const double stepPpq=0.25; // 16th-note step
    auto* ph=getPlayHead();
    juce::ignoreUnused(ph);
    int startStep=(int)std::floor(ppqStart/stepPpq);
    int endStep=(int)std::floor(ppqEnd/stepPpq);
    if(lastGeneratedPpq<0.0 || ppqStart+0.0001<lastGeneratedPpq) {
        lastGeneratedStep=startStep-1;
        for(int n:lastNotes) if(n>=0) midi.addEvent(juce::MidiMessage::noteOff(1,n),0);
        lastNotes.fill(-1);
    }
    for(int absoluteStep=std::max(startStep,lastGeneratedStep+1); absoluteStep<=endStep; ++absoluteStep) {
        double stepPpqPos=absoluteStep*stepPpq;
        if((absoluteStep&1)!=0) stepPpqPos+=swing*stepPpq*0.45;
        int offset=(int)std::round((stepPpqPos-ppqStart)/ppqPerSample);
        if(offset<0 || offset>=numSamples) continue;
        int idx=((absoluteStep%32)+32)%32;
        int old=lastNotes[0];
        if(old>=0) midi.addEvent(juce::MidiMessage::noteOff(1,old),offset);
        int note=melody[(size_t)idx];
        if(note>=0) {
            midi.addEvent(juce::MidiMessage::noteOn(1,note,(juce::uint8)100),offset);
            lastNotes[0]=note;
        } else lastNotes[0]=-1;

        // Chord/bass at each beat; drums on channel 10.
        if(idx%4==0) {
            int barSlot=(idx/4)%8;
            for(int n=1;n<4;++n) if(lastNotes[(size_t)n]>=0)
                midi.addEvent(juce::MidiMessage::noteOff(n+1,lastNotes[(size_t)n]),offset);
            for(int n=0;n<3;++n) {
                int pitch=chordNotes[(size_t)barSlot][(size_t)n];
                midi.addEvent(juce::MidiMessage::noteOn(2,pitch,(juce::uint8)70),offset);
            }
            lastNotes[1]=chordNotes[(size_t)barSlot][0];
            lastNotes[2]=bassNotes[(size_t)barSlot];
            midi.addEvent(juce::MidiMessage::noteOn(3,lastNotes[2],(juce::uint8)100),offset);
        }
        int ds=idx%16;
        if(drumSteps[(size_t)ds]) {
            int drum=(ds%4==0)?36:((ds%4==2)?38:42);
            midi.addEvent(juce::MidiMessage::noteOn(10,drum,(juce::uint8)95),offset);
            midi.addEvent(juce::MidiMessage::noteOff(10,drum),juce::jmin(numSamples-1,offset+12));
        }
        lastGeneratedStep=absoluteStep;
    }
    lastGeneratedPpq=ppqEnd;
}
void WonderMelodyGeneratorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    juce::AudioPlayHead::CurrentPositionInfo pos;
    bool have=false;
    if(auto* ph=getPlayHead()) have=ph->getCurrentPosition(pos);
    scheduleGeneratedMidi(midi,buffer.getNumSamples(),pos,have);

    // Consume MIDI in chronological order, then render voices.
    for(const auto metadata:midi) {
        auto msg=metadata.getMessage();
        if(msg.isNoteOn()) noteOn(msg.getNoteNumber(),msg.getChannel(),msg.getFloatVelocity());
        else if(msg.isNoteOff()) noteOff(msg.getNoteNumber(),msg.getChannel());
    }
    renderSynth(buffer,buffer.getNumSamples());
}
void WonderMelodyGeneratorAudioProcessor::generatePattern()
{
    const int scale[]={0,2,3,5,7,8,10};
    std::uniform_int_distribution<int> pick(0,6), oct(0,1), chance(0,99);
    int previous=60+rootNote+7;
    for(int i=0;i<32;++i) {
        if(chance(rng)<density*22) {
            int candidate=60+rootNote+scale[pick(rng)]+oct(rng)*12;
            if(std::abs(candidate-previous)>9) candidate=previous+(candidate>previous?-2:2);
            melody[(size_t)i]=juce::jlimit(48,88,candidate); previous=melody[(size_t)i];
        } else melody[(size_t)i]=-1;
    }
    generateChordBass(); randomizeDrums();
}
void WonderMelodyGeneratorAudioProcessor::generateChordBass()
{
    // Minor progression i - VI - III - VII, with inversions selected for smooth voice leading.
    const int roots[]={0,8,3,10,0,8,3,10};
    const int inv[]={0,1,2,0,1,2,0,1};
    for(int i=0;i<8;++i) {
        int r=(rootNote+roots[i])%12;
        int triad[3]={r+48,r+51,r+55};
        for(int j=0;j<inv[i];++j) { triad[0]+=12; int temp=triad[0]; triad[0]=triad[1]; triad[1]=triad[2]; triad[2]=temp; }
        chordNotes[(size_t)i]={(juce::jlimit(48,76,triad[0])),(juce::jlimit(52,80,triad[1])),(juce::jlimit(55,84,triad[2]))};
        bassNotes[(size_t)i]=36+r;
    }
}
void WonderMelodyGeneratorAudioProcessor::randomizeDrums()
{
    std::uniform_int_distribution<int> c(0,99);
    for(int i=0;i<16;++i) drumSteps[(size_t)i]=(i%4==0)||(i%4==2?c(rng)<72:c(rng)<18);
}
void WonderMelodyGeneratorAudioProcessor::setDrumStep(int step,bool enabled){if(step>=0&&step<16)drumSteps[(size_t)step]=enabled;}
bool WonderMelodyGeneratorAudioProcessor::getDrumStep(int step) const{return step>=0&&step<16?drumSteps[(size_t)step]:false;}
void WonderMelodyGeneratorAudioProcessor::exportAllMidi(const juce::File& target)
{
    juce::MidiFile mf; mf.setTicksPerQuarterNote(960);
    juce::MidiMessageSequence m,c,b,d;
    auto add=[](juce::MidiMessageSequence& s,int note,int ch,double start,double len,int vel){
        s.addEvent(juce::MidiMessage::noteOn(ch,note,(juce::uint8)vel),start);
        s.addEvent(juce::MidiMessage::noteOff(ch,note),start+len);
    };
    for(int i=0;i<32;++i) if(melody[(size_t)i]>=0) {
        double start=i*240.0+((i&1)?swing*120.0:0.0); add(m,melody[(size_t)i],1,start,180.0,100);
    }
    for(int i=0;i<8;++i) {
        for(int n:chordNotes[(size_t)i]) add(c,n,2,i*960.0,900.0,78);
        add(b,bassNotes[(size_t)i],3,i*960.0,760.0,105);
    }
    for(int bar=0;bar<2;++bar) for(int i=0;i<16;++i) if(drumSteps[(size_t)i]) {
        int note=(i%4==0)?36:((i%4==2)?38:42);
        add(d,note,10,(bar*16+i)*240.0,90.0,100);
    }
    m.updateMatchedPairs(); c.updateMatchedPairs(); b.updateMatchedPairs(); d.updateMatchedPairs();
    mf.addTrack(m); mf.addTrack(c); mf.addTrack(b); mf.addTrack(d);
    juce::FileOutputStream stream(target); if(stream.openedOk()) mf.writeTo(stream);
}
void WonderMelodyGeneratorAudioProcessor::requestAiGeneration(const juce::String& prompt)
{
    { const juce::ScopedLock lock(aiLock); aiStatus="Connecting to AI service..."; }
    auto endpoint=aiEndpoint; auto model=aiModel; auto promptCopy=prompt;
    const int key=rootNote, bpm=tempo;
    juce::Thread::launch([this,endpoint,model,promptCopy,key,bpm]() {
        juce::DynamicObject::Ptr payload=new juce::DynamicObject();
        payload->setProperty("prompt",promptCopy);
        payload->setProperty("model",model);
        payload->setProperty("key",key);
        payload->setProperty("tempo",bpm);
        auto request=juce::URL(endpoint).withPOSTData(juce::JSON::toString(juce::var(payload.get())));
        juce::URL::InputStreamOptions opts(juce::URL::ParameterHandling::inAddress);
        opts.withConnectionTimeoutMs(15000).withExtraHeaders("Content-Type: application/json\r\n");
        std::unique_ptr<juce::InputStream> in(request.createInputStream(opts));
        juce::var response;
        juce::String error;
        if(!in) error="AI service unavailable. Start ai_service/server.py and verify the endpoint.";
        else {
            response=juce::JSON::parse(in->readEntireStreamAsString());
            if(response.isObject() && response.getProperty("error",juce::var()).isString())
                error=response.getProperty("error",juce::var()).toString();
            else if(!response.isObject()) error="AI returned invalid JSON.";
        }
        const juce::ScopedLock lock(aiLock);
        if(error.isNotEmpty()) aiStatus="AI error: "+error;
        else { pendingAiData=response; aiStatus="AI result ready to load"; }
    });
}
bool WonderMelodyGeneratorAudioProcessor::consumePendingAiResult()
{
    juce::var result;
    {
        const juce::ScopedLock lock(aiLock);
        if(pendingAiData.isVoid()) return false;
        result=pendingAiData; pendingAiData=juce::var();
    }
    applyAiJson(result);
    return true;
}
juce::String WonderMelodyGeneratorAudioProcessor::getAiStatus() const
{
    const juce::ScopedLock lock(aiLock);
    return aiStatus;
}
void WonderMelodyGeneratorAudioProcessor::applyAiJson(const juce::var& data)
{
    auto* obj=data.getDynamicObject(); if(!obj) return;
    auto* ma=obj->getProperty("melody").getArray();
    auto* ba=obj->getProperty("bass").getArray();
    auto* da=obj->getProperty("drums").getArray();
    auto* ca=obj->getProperty("chords").getArray();
    if(ma && ma->size()==32) for(int i=0;i<32;++i) melody[(size_t)i]=juce::jlimit(-1,127,(int)(*ma)[i]);
    if(ba && ba->size()==8) for(int i=0;i<8;++i) bassNotes[(size_t)i]=juce::jlimit(0,127,(int)(*ba)[i]);
    if(da && da->size()==16) for(int i=0;i<16;++i) drumSteps[(size_t)i]=(bool)(*da)[i];
    if(ca && ca->size()==8) for(int i=0;i<8;++i) if(auto* triad=(*ca)[i].getArray();triad && triad->size()>=3)
        chordNotes[(size_t)i]={(int)(*triad)[0],(int)(*triad)[1],(int)(*triad)[2]};
}
void WonderMelodyGeneratorAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream s(dest,true);
    s.writeInt(rootNote);s.writeInt(tempo);s.writeInt(density);s.writeFloat(swing);s.writeInt(polyphony);
    s.writeFloat(filterCutoff.load());s.writeFloat(attackMs.load());s.writeFloat(releaseMs.load());s.writeInt(preset.load());
    for(auto x:melody)s.writeInt(x);for(auto x:bassNotes)s.writeInt(x);for(auto x:drumSteps)s.writeBool(x);
    for(auto row:chordNotes)for(auto x:row)s.writeInt(x);
}
void WonderMelodyGeneratorAudioProcessor::setStateInformation(const void* data,int size)
{
    juce::MemoryInputStream s(data,(size_t)size,false);if(s.getNumBytesRemaining()<36)return;
    rootNote=s.readInt();tempo=s.readInt();density=s.readInt();swing=s.readFloat();polyphony=s.readInt();
    filterCutoff=s.readFloat();attackMs=s.readFloat();releaseMs=s.readFloat();preset=s.readInt();
    for(auto& x:melody)x=s.readInt();for(auto& x:bassNotes)x=s.readInt();for(auto& x:drumSteps)x=s.readBool();
    for(auto& row:chordNotes)for(auto& x:row)x=s.readInt();
}
juce::AudioProcessorEditor* WonderMelodyGeneratorAudioProcessor::createEditor(){return new WonderMelodyGeneratorAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new WonderMelodyGeneratorAudioProcessor();}
