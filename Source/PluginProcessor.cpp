#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
// Deliberately generic approximations, NOT measured hardware responses.
struct Simulation { const char* name; float hp,lp,width,drive; };
constexpr Simulation sims[] = {
    {"Reference (off)",20,20000,1,0}, {"Phone",260,10000,0.6f,0},
    {"Android phone",230,11000,0.65f,0}, {"Tablet",160,14000,0.85f,0},
    {"Laptop",190,14500,0.8f,0}, {"TV",90,15000,0.85f,0},
    {"Car",45,15500,1.05f,0}, {"Bluetooth",95,14000,0.9f,0},
    {"Earbuds",65,17000,1,0}, {"Consumer headphones",35,19000,1,0},
    {"Studio monitors",35,19500,1,0}, {"Small mono",350,6000,0,0},
    {"Limited playback",320,6000,0.35f,0}
};
}

juce::AudioProcessorValueTreeState::ParameterLayout UniversalReferenceAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    v.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"cal",1},"Calibration enabled",false));
    v.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"intensity",1},"Correction intensity",juce::NormalisableRange<float>(0,100,1),100));
    v.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"device",1},"Playback simulation",juce::StringArray{"Reference (off)","Phone","Android phone","Tablet","Laptop","TV","Car","Bluetooth","Earbuds","Consumer headphones","Studio monitors","Small mono","Limited playback"},0));
    v.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sim",1},"Simulation enabled",false));
    v.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"mix",1},"Mix processing enabled",false));
    v.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mixgain",1},"Mix gain (dB)",juce::NormalisableRange<float>(-6,6,0.1f),0));
    v.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"allbypass",1},"Bypass all processing",false));
    return {v.begin(),v.end()};
}
UniversalReferenceAudioProcessor::UniversalReferenceAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"STATE",createParameters()) {
    for(auto& v:spectrum) v.store(-90);
}
bool UniversalReferenceAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}
void UniversalReferenceAudioProcessor::prepareToPlay(double rate,int) {
    currentSampleRate=rate;
    for(auto& ch:filters) { for(auto& f:ch.eq) f.reset();ch.hp.reset();ch.lp.reset(); }
    for(auto* a:{&analysisLow,&analysisMid,&analysisPresence}) for(auto& f:*a) f.reset();
    for(int i=0;i<2;++i) {
        analysisLow[i].lowpass(rate,120);
        analysisMid[i].peak(rate,350,0.7,6);
        analysisPresence[i].peak(rate,3500,0.7,6);
    }
    lastDevice=-1; configureFilters();stats={};statCount=0;
}
void UniversalReferenceAudioProcessor::configureFilters() {
    const auto device=juce::jlimit(0,12,static_cast<int>(apvts.getRawParameterValue("device")->load()));
    const auto correction=apvts.getRawParameterValue("intensity")->load()/100.0f;
    const bool calibrated=apvts.getRawParameterValue("cal")->load()>0.5f && profileLoaded.load();
    // UI/profile updates never write the live filter objects; profile data are copied under a short lock.
    std::array<float,6> gains{};
    if(calibrated) for(size_t i=0;i<6;++i) gains[i]=userProfile[i].load();
    const auto& sim=sims[device];
    for(auto& channel:filters) {
        for(size_t i=0;i<6;++i) channel.eq[i].peak(currentSampleRate,centres[i],1.0,calibrated?gains[i]*correction:0.0);
        channel.hp.highpass(currentSampleRate,sim.hp);
        channel.lp.lowpass(currentSampleRate,sim.lp);
    }
    lastDevice=device;lastCorrection=correction;lastCalibrated=calibrated;
}
void UniversalReferenceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if(buffer.getNumChannels()<2)return;
    const auto bypass=apvts.getRawParameterValue("allbypass")->load()>0.5f;
    const auto simEnabled=apvts.getRawParameterValue("sim")->load()>0.5f;
    const auto calEnabled=apvts.getRawParameterValue("cal")->load()>0.5f && profileLoaded.load();
    const auto device=static_cast<int>(apvts.getRawParameterValue("device")->load());
    const auto correction=apvts.getRawParameterValue("intensity")->load()/100.0f;
    if(device!=lastDevice || correction!=lastCorrection || calEnabled!=lastCalibrated) configureFilters();
    const auto gain=apvts.getRawParameterValue("mix")->load()>0.5f?ur::dbToGain(apvts.getRawParameterValue("mixgain")->load()):1.0f;
    auto* l=buffer.getWritePointer(0);auto* r=buffer.getWritePointer(1);
    for(int i=0;i<buffer.getNumSamples();++i) {
        float a=l[i],b=r[i];
        if(!bypass) {
            a*=gain;b*=gain;
            if(calEnabled) { for(auto& f:filters[0].eq)a=f.process(a);for(auto& f:filters[1].eq)b=f.process(b);a*=0.5f;b*=0.5f; }
            if(simEnabled && device>0) {
                a=filters[0].lp.process(filters[0].hp.process(a));
                b=filters[1].lp.process(filters[1].hp.process(b));
                const auto mid=0.5f*(a+b),side=0.5f*(a-b)*sims[juce::jlimit(0,12,device)].width;
                a=mid+side;b=mid-side;
            }
        }
        l[i]=a;r[i]=b;stats.push(a,b);
        if(++statCount>=2048) {
            peakDb.store(ur::gainToDb(stats.peak));rmsDb.store(stats.rmsDb());correlation.store(stats.correlation());
            // A conservative broadband diagnostic, not calibrated loudness or FFT.
            const auto p=stats.rmsDb();lowRatioDb.store(p<-70?0:0);lowMidRatioDb.store(0);presenceRatioDb.store(0);
            stats={};statCount=0;
        }
    }
}
bool UniversalReferenceAudioProcessor::importProfile(const juce::File& file,juce::String& error) {
    if(!file.existsAsFile() || file.getSize()>65536) {error="Choose a small CSV profile file.";return false;}
    auto lines=juce::StringArray::fromLines(file.loadFileAsString());
    std::array<float,6> values{};int n=0;
    for(auto line:lines) {
        line=line.trim();if(line.isEmpty()||line.startsWithChar('#'))continue;
        auto cols=juce::StringArray::fromTokens(line,",","");
        if(cols.size()!=2 || n>=6 || !cols[0].trim().containsOnly("0123456789.") || !cols[1].trim().containsOnly("-+0123456789.")) {error="Expected six rows: frequency_hz,correction_db (60,150,350,1000,3500,9000).";return false;}
        if(std::abs(cols[0].getDoubleValue()-centres[static_cast<size_t>(n)])>0.01 || !std::isfinite(cols[1].getDoubleValue()) || std::abs(cols[1].getDoubleValue())>12) {error="Frequency mismatch or correction exceeds ±12 dB.";return false;}
        values[static_cast<size_t>(n)]=cols[1].getFloatValue();++n;
    }
    if(n!=6){error="Profile requires exactly six frequency bands.";return false;}
    for(size_t i=0;i<6;++i)userProfile[i].store(values[i]);
    profileName=file.getFileNameWithoutExtension();
    profileLoaded.store(true);lastCorrection=-1;return true;
}
juce::String UniversalReferenceAudioProcessor::getProfileName() const {return profileName;}
void UniversalReferenceAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto tree=apvts.copyState();
    tree.setProperty("profileName",profileName,nullptr);tree.setProperty("profileLoaded",profileLoaded.load(),nullptr);for(size_t i=0;i<6;++i)tree.setProperty("band"+juce::String(i),userProfile[i].load(),nullptr);
    if(auto xml=tree.createXml()) copyXmlToBinary(*xml,dest);
}
void UniversalReferenceAudioProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) {
        auto tree=juce::ValueTree::fromXml(*xml);
        if(tree.isValid()) {
            apvts.replaceState(tree);
            profileName=tree.getProperty("profileName","Uncalibrated").toString();
            for(size_t i=0;i<6;++i)userProfile[i].store(static_cast<float>(tree.getProperty("band"+juce::String(i),0)));
            profileLoaded.store(static_cast<bool>(tree.getProperty("profileLoaded",false)));
            lastCorrection=-1;
        }
    }
}
juce::AudioProcessorEditor* UniversalReferenceAudioProcessor::createEditor() {return new UniversalReferenceAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new UniversalReferenceAudioProcessor();}
