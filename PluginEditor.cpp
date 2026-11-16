#include "PluginEditor.h"

MasteringPro2AudioProcessorEditor::MasteringPro2AudioProcessorEditor(MasteringPro2AudioProcessor& p)
 : AudioProcessorEditor(&p),processor(p)
{
    setSize(1040,690);
    addKnob("input","INPUT"); addKnob("low","LOW"); addKnob("lowmid","LOW MID");
    addKnob("mid","MID"); addKnob("himid","HIGH MID"); addKnob("high","HIGH");
    addKnob("threshold","THRESHOLD"); addKnob("ratio","RATIO"); addKnob("attack","ATTACK");
    addKnob("release","RELEASE"); addKnob("width","STEREO WIDTH"); addKnob("saturation","SATURATION");
    addKnob("ceiling","TP CEILING"); addKnob("limiterRelease","LIMITER REL."); addKnob("output","OUTPUT");
    presetBox.addItem("Transparent Master",1); presetBox.addItem("Streaming",2);
    presetBox.addItem("Pop / Vocal",3); presetBox.addItem("EDM",4); presetBox.addItem("Hip-Hop",5);
    presetBox.addItem("Acoustic",6); addAndMakeVisible(presetBox);
    loadPreset.onClick=[this]{applyPreset(presetBox.getSelectedId());};
    resetMeter.onClick=[this]{processor.resetLoudness();};
    saveA.onClick=[this]{storeSlot(slotA);}; loadA.onClick=[this]{recallSlot(slotA);};
    saveB.onClick=[this]{storeSlot(slotB);}; loadB.onClick=[this]{recallSlot(slotB);};
    for(auto* c:{&loadPreset,&resetMeter,&saveA,&loadA,&saveB,&loadB}) addAndMakeVisible(c);
    meterLabel.setColour(juce::Label::textColourId,juce::Colours::white);
    meterLabel.setFont(juce::Font(juce::FontOptions(14.0f,juce::Font::bold)));
    addAndMakeVisible(meterLabel);
    startTimerHz(20);
}
void MasteringPro2AudioProcessorEditor::addKnob(const juce::String& id,const juce::String& name)
{
    auto k=std::make_unique<Knob>();
    k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,66,18);
    k->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff20c7b5));
    k->label.setText(name,juce::dontSendNotification); k->label.setJustificationType(juce::Justification::centred);
    k->label.setColour(juce::Label::textColourId,juce::Colour(0xffdce8eb));
    k->attach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,k->slider);
    addAndMakeVisible(k->slider);addAndMakeVisible(k->label);knobs.push_back(std::move(k));
}
void MasteringPro2AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080f16));
    g.setColour(juce::Colour(0xff14232e));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),16);
    g.setColour(juce::Colour(0xff20c7b5));g.fillRect(30,25,5,44);
    g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(29,juce::Font::bold)));
    g.drawText("MASTERING PRO 2.0",48,23,420,35,juce::Justification::left);
    g.setColour(juce::Colour(0xff8fa8b5));g.setFont(juce::Font(juce::FontOptions(12)));
    g.drawText("STEREO MASTERING SUITE  |  VST3  |  64-BIT",50,57,450,18,juce::Justification::left);
    g.setColour(juce::Colour(0xff20c7b5));g.drawLine(28,91,(float)getWidth()-28,91,1);
    auto spec=juce::Rectangle<int>(35,130,350,150);
    g.setColour(juce::Colour(0xff0b141c));g.fillRoundedRectangle(spec.toFloat(),8);
    g.setColour(juce::Colour(0xff29424e));
    for(int i=1;i<5;++i)g.drawHorizontalLine(spec.getY()+i*spec.getHeight()/5,(float)spec.getX(),(float)spec.getRight());
    for(int i=1;i<8;++i)g.drawVerticalLine(spec.getX()+i*spec.getWidth()/8,(float)spec.getY(),(float)spec.getBottom());
    if(!spectrum.isNull())g.drawImage(spectrum,spec.toFloat());
    g.setColour(juce::Colour(0xff8fa8b5));g.drawText("REAL-TIME SPECTRUM",spec.getX(),spec.getBottom()+5,spec.getWidth(),18,juce::Justification::centredLeft);
}
void MasteringPro2AudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(28);r.removeFromTop(78);
    auto top=r.removeFromTop(38);presetBox.setBounds(top.removeFromLeft(190));loadPreset.setBounds(top.removeFromLeft(125).reduced(3));
    resetMeter.setBounds(top.removeFromLeft(120).reduced(3));saveA.setBounds(top.removeFromLeft(85).reduced(3));
    loadA.setBounds(top.removeFromLeft(85).reduced(3));saveB.setBounds(top.removeFromLeft(85).reduced(3));loadB.setBounds(top.removeFromLeft(85).reduced(3));
    meterLabel.setBounds(r.removeFromTop(28));
    auto specArea=r.removeFromTop(190); // spectrum drawn at fixed upper-left; controls occupy lower rows
    juce::ignoreUnused(specArea);
    const int cols=8, rows=2, w=r.getWidth()/cols, h=r.getHeight()/rows;
    for(size_t i=0;i<knobs.size();++i){int row=(int)i/cols,col=(int)i%cols;auto cell=juce::Rectangle<int>(r.getX()+col*w,r.getY()+row*h,w,h).reduced(4);knobs[i]->label.setBounds(cell.removeFromTop(21));knobs[i]->slider.setBounds(cell);}
}
void MasteringPro2AudioProcessorEditor::timerCallback()
{
    auto l=processor.getIntegratedLUFS(),s=processor.getShortTermLUFS(),tp=processor.getTruePeakDb(),c=processor.getStereoCorrelation();
    meterLabel.setText("Integrated LUFS (estimate): "+juce::String(l,1)+"   Short-term: "+juce::String(s,1)+
        "   True Peak sample estimate: "+juce::String(tp,1)+" dBTP   Correlation: "+juce::String(c,2),juce::dontSendNotification);
    std::array<float,2048> data{};
    if(processor.getNextFFTBlock(data.data()))
    {
        spectrum=juce::Image(juce::Image::RGB,350,150,true);
        juce::Graphics g(spectrum);g.fillAll(juce::Colours::transparentBlack);
        juce::Path path;path.startNewSubPath(0,145);
        for(int x=0;x<350;++x){int bin=juce::jlimit(1,1023,(int)std::exp((x/350.0)*std::log(1024.0)));float v=juce::jlimit(0.0f,1.0f,std::log10(1.0f+data[(size_t)bin])*0.22f);path.lineTo((float)x,145.0f-v*130.0f);}
        g.setColour(juce::Colour(0xff20c7b5));g.strokePath(path,juce::PathStrokeType(2.0f));
        repaint(25,130,370,175);
    }
}
void MasteringPro2AudioProcessorEditor::applyPreset(int id)
{
    auto set=[this](const char* key,float v){if(auto* p=processor.parameters.getParameter(key))p->setValueNotifyingHost(p->convertTo0to1(v));};
    // Conservative starting points; always audition and level-match.
    switch(id){
      case 1:set("low",0);set("lowmid",0);set("mid",0);set("himid",0);set("high",0);set("threshold",-18);set("ratio",1.4f);set("width",1);set("saturation",0);set("ceiling",-1);break;
      case 2:set("threshold",-14);set("ratio",1.8f);set("width",1.05f);set("saturation",0.04f);set("ceiling",-1);break;
      case 3:set("low",0.5f);set("lowmid",-0.5f);set("himid",0.8f);set("high",0.5f);set("threshold",-16);set("ratio",1.8f);set("width",1.08f);set("ceiling",-1);break;
      case 4:set("low",1);set("lowmid",0.5f);set("mid",-0.5f);set("high",1);set("threshold",-12);set("ratio",2.5f);set("width",1.12f);set("saturation",0.08f);set("ceiling",-1);break;
      case 5:set("low",1.2f);set("lowmid",0.5f);set("mid",-0.3f);set("high",0.3f);set("threshold",-13);set("ratio",2.2f);set("width",1);set("saturation",0.06f);set("ceiling",-1);break;
      case 6:set("low",0.3f);set("lowmid",0);set("mid",0.2f);set("himid",0.3f);set("high",0.2f);set("threshold",-20);set("ratio",1.4f);set("width",1);set("saturation",0);set("ceiling",-1);break;
      default:break;
    }
}
void MasteringPro2AudioProcessorEditor::storeSlot(juce::ValueTree& slot){slot=processor.parameters.copyState();}
void MasteringPro2AudioProcessorEditor::recallSlot(const juce::ValueTree& slot){if(slot.isValid())processor.parameters.replaceState(slot);}
