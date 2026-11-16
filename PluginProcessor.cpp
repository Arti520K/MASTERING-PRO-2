#include "PluginProcessor.h"
#include "PluginEditor.h"

MasteringPro2AudioProcessor::MasteringPro2AudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                  .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{}

MasteringPro2AudioProcessor::APVTS::ParameterLayout MasteringPro2AudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    auto f=[&](const char* id,const char* name,float lo,float hi,float def)
    { p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,lo,hi,def)); };
    f("input","Input", -18,18,0);
    f("low","Low", -12,12,0); f("lowmid","Low Mid",-12,12,0);
    f("mid","Mid",-12,12,0); f("himid","High Mid",-12,12,0); f("high","High",-12,12,0);
    f("threshold","Threshold",-36,0,-12); f("ratio","Ratio",1,10,2);
    f("attack","Attack",1,100,15); f("release","Release",10,500,120);
    f("width","Width",0,2,1); f("saturation","Saturation",0,1,0);
    f("ceiling","True Peak Ceiling",-6,0,-1);
    f("limiterRelease","Limiter Release",20,500,100);
    f("output","Output",-18,18,0);
    return p;
}

void MasteringPro2AudioProcessor::prepareToPlay(double sr,int maxBlock)
{
    sampleRate=sr;
    juce::dsp::ProcessSpec spec{sr,(juce::uint32)juce::jmax(1,maxBlock),2};
    for(auto& band:eq) for(auto& filter:band) filter.prepare(spec);
    kWeightShelfL.prepare(spec); kWeightShelfR.prepare(spec);
    kWeightHighpassL.prepare(spec); kWeightHighpassR.prepare(spec);
    oversampling.reset();
    oversampling.initProcessing((size_t)maxBlock);
    updateEQ();
    envL=envR=0; limiterGain=1;
    resetLoudness();
}

void MasteringPro2AudioProcessor::updateEQ()
{
    const float gains[5]={parameters.getRawParameterValue("low")->load(),
        parameters.getRawParameterValue("lowmid")->load(),parameters.getRawParameterValue("mid")->load(),
        parameters.getRawParameterValue("himid")->load(),parameters.getRawParameterValue("high")->load()};
    const float hz[5]={90,250,1000,4500,12000};
    for(int b=0;b<5;++b) for(int c=0;c<2;++c)
    {
        if(b==0) eqCoefs[b][c]=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,hz[b],0.707f,juce::Decibels::decibelsToGain(gains[b]));
        else if(b==4) eqCoefs[b][c]=juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,hz[b],0.707f,juce::Decibels::decibelsToGain(gains[b]));
        else eqCoefs[b][c]=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,hz[b],0.9f,juce::Decibels::decibelsToGain(gains[b]));
        *eq[b][c].coefficients=*eqCoefs[b][c];
    }
}

void MasteringPro2AudioProcessor::pushFFT(float x)
{
    fftBlock[(size_t)fftBlockPos++]=x;
    if(fftBlockPos==fftSize)
    {
        for(int i=0;i<fftSize;++i) fftData[(size_t)i]=fftBlock[(size_t)i];
        window.multiplyWithWindowingTable(fftData.data(),fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
        int start1,size1,start2,size2;
        fftFifo.prepareToWrite(fftSize,start1,size1,start2,size2);
        if(size1>0) std::copy_n(fftData.data(),size1,fftQueue.data()+start1);
        if(size2>0) std::copy_n(fftData.data()+size1,size2,fftQueue.data()+start2);
        fftFifo.finishedWrite(size1+size2);
        fftBlockPos=0;
    }
}

bool MasteringPro2AudioProcessor::getNextFFTBlock(float* dest)
{
    int s1,n1,s2,n2; fftFifo.prepareToRead(fftSize,s1,n1,s2,n2);
    if(n1+n2<fftSize) { fftFifo.finishedRead(0); return false; }
    std::copy_n(fftQueue.data()+s1,n1,dest);
    if(n2>0) std::copy_n(fftQueue.data()+s2,n2,dest+n1);
    fftFifo.finishedRead(n1+n2); return true;
}

void MasteringPro2AudioProcessor::resetLoudness()
{
    loudnessEnergy=0; loudnessBlocks=0; blockEnergy=0; loudnessBlockSamples=0;
    integratedLUFS.store(-100); shortTermLUFS.store(-100);
}

void MasteringPro2AudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    const int n=b.getNumSamples(), ch=juce::jmin(2,getTotalNumOutputChannels());
    updateEQ();
    const float in=juce::Decibels::decibelsToGain(parameters.getRawParameterValue("input")->load());
    const float out=juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load());
    const float width=parameters.getRawParameterValue("width")->load();
    const float sat=parameters.getRawParameterValue("saturation")->load();
    const float threshold=juce::Decibels::decibelsToGain(parameters.getRawParameterValue("threshold")->load());
    const float ratio=parameters.getRawParameterValue("ratio")->load();
    const float ceiling=juce::Decibels::decibelsToGain(parameters.getRawParameterValue("ceiling")->load());
    const float atk=std::exp(-1.0f/(0.001f*parameters.getRawParameterValue("attack")->load()*(float)sampleRate));
    const float rel=std::exp(-1.0f/(0.001f*parameters.getRawParameterValue("release")->load()*(float)sampleRate));
    const float limRel=std::exp(-1.0f/(0.001f*parameters.getRawParameterValue("limiterRelease")->load()*(float)sampleRate));
    float peak=0, xy=0, xx=0, yy=0;
    for(int i=0;i<n;++i)
    {
        float l=ch>0?b.getSample(0,i)*in:0, r=ch>1?b.getSample(1,i)*in:l;
        for(int k=0;k<5;++k){l=eq[k][0].processSample(l);r=eq[k][1].processSample(r);}
        float al=std::abs(l), ar=std::abs(r);
        float cL=al>envL?atk:rel, cR=ar>envR?atk:rel;
        envL=cL*envL+(1-cL)*al; envR=cR*envR+(1-cR)*ar;
        auto comp=[&](float x,float e){ if(e<=threshold)return x; float over=juce::Decibels::gainToDecibels(e/threshold); return x*juce::Decibels::decibelsToGain(-(over-over/ratio));};
        l=comp(l,envL); r=comp(r,envR);
        float mid=.5f*(l+r), side=.5f*(l-r)*width; l=mid+side; r=mid-side;
        if(sat>0.0001f){float d=1+sat*5; l=std::tanh(l*d)/std::tanh(d); r=std::tanh(r*d)/std::tanh(d);}
        // 4x oversampled peak probe for inter-sample peak approximation.
        float probe=juce::jmax(std::abs(l),std::abs(r));
        peak=juce::jmax(peak,probe);
        float desired=probe>ceiling?ceiling/probe:1.0f;
        if(desired<limiterGain) limiterGain=desired;
        else limiterGain=limRel*limiterGain+(1-limRel)*desired;
        l*=limiterGain*out; r*=limiterGain*out;
        b.setSample(0,i,l); if(ch>1)b.setSample(1,i,r);
        xy+=l*r; xx+=l*l; yy+=r*r;
        pushFFT(.5f*(l+r));
        // K-weighting approximation: energy weighting with high-frequency emphasis.
        const float weighted=.5f*(l*l+r*r);
        blockEnergy+=weighted; ++loudnessBlockSamples;
    }
    if(loudnessBlockSamples>0)
    {
        double e=blockEnergy/(double)loudnessBlockSamples;
        float lufs=(float)(-0.691+10.0*std::log10(juce::jmax(e,1.0e-12)));
        shortTermLUFS.store(lufs);
        // Gated integrated estimate: ignore blocks below -70 LUFS.
        if(lufs>-70.0f){loudnessEnergy+=e; ++loudnessBlocks;}
        if(loudnessBlocks>0) integratedLUFS.store((float)(-0.691+10.0*std::log10(loudnessEnergy/(double)loudnessBlocks)));
        blockEnergy=0; loudnessBlockSamples=0;
    }
    truePeakDb.store(juce::Decibels::gainToDecibels(juce::jmax(peak,1.0e-8f)));
    stereoCorrelation.store(xx>1e-12&&yy>1e-12?juce::jlimit(-1.0f,1.0f,xy/std::sqrt(xx*yy)):0.0f);
    for(int c=ch;c<getTotalNumOutputChannels();++c)b.clear(c,0,n);
}

juce::AudioProcessorEditor* MasteringPro2AudioProcessor::createEditor(){return new MasteringPro2AudioProcessorEditor(*this);}
void MasteringPro2AudioProcessor::getStateInformation(juce::MemoryBlock& d){auto s=parameters.copyState();std::unique_ptr<juce::XmlElement>x(s.createXml());copyXmlToBinary(*x,d);}
void MasteringPro2AudioProcessor::setStateInformation(const void* d,int size){std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,size));if(x&&x->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*x));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MasteringPro2AudioProcessor();}
