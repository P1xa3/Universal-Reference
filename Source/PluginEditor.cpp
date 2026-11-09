#include "PluginEditor.h"

namespace {
constexpr auto cream=0xfff7f3ea, white=0xfffffcf6, ink=0xff34312d, taupe=0xff898176;
void panel(juce::Graphics& g,juce::Rectangle<float> area,juce::Colour colour,float radius=28) {
    g.setColour(juce::Colours::black.withAlpha(.045f));g.fillRoundedRectangle(area.translated(0,4),radius);
    g.setColour(colour);g.fillRoundedRectangle(area,radius);
}
void caption(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float font,juce::Colour c,bool bold=false) {
    g.setColour(c);
    g.setFont(juce::Font(juce::FontOptions(font,bold?juce::Font::bold:juce::Font::plain)));
    g.drawFittedText(s,r.toNearestInt(),juce::Justification::centred,1);
}
}
HomeCard::HomeCard(const juce::String& title,juce::Colour c,int s):juce::Button(title),colour(c),symbol(s) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void HomeCard::paintButton(juce::Graphics& g,bool hover,bool pressed) {
    auto r=getLocalBounds().toFloat().reduced(pressed?8:5);
    if(hover && !pressed)r=r.reduced(1);
    panel(g,r,juce::Colour(white),30);
    const float cx=r.getCentreX(),cy=r.getY()+86;
    g.setColour(colour);g.fillRoundedRectangle(cx-50,cy-50,100,100,31);
    g.setColour(juce::Colour(ink));
    if(symbol==0) {
        juce::Path p;p.startNewSubPath(cx-28,cy+6);p.lineTo(cx-28,cy-6);
        p.cubicTo(cx-28,cy-48,cx+28,cy-48,cx+28,cy-6);p.lineTo(cx+28,cy+6);
        g.strokePath(p,juce::PathStrokeType(5,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.fillRoundedRectangle(cx-35,cy+1,15,29,7);g.fillRoundedRectangle(cx+20,cy+1,15,29,7);
    } else if(symbol==1) {
        juce::Path p;p.startNewSubPath(cx-34,cy);
        for(int x=-32;x<=34;x+=2)p.lineTo(cx+x,cy+std::sin(x*.25f)*std::sin((x+34)*juce::MathConstants<float>::pi/68)*27);
        g.strokePath(p,juce::PathStrokeType(5,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    } else {
        g.fillRoundedRectangle(cx-28,cy-33,56,66,11);
        g.setColour(colour);g.fillEllipse(cx-13,cy-10,26,26);g.fillEllipse(cx-4,cy-24,8,8);
    }
    caption(g,getName(),{r.getX()+8,r.getBottom()-74,r.getWidth()-16,42},26,juce::Colour(ink),true);
    caption(g,"Explore  →",{r.getX()+8,r.getBottom()-35,r.getWidth()-16,22},14,juce::Colour(taupe));
}

UniversalReferenceAudioProcessorEditor::UniversalReferenceAudioProcessorEditor(UniversalReferenceAudioProcessor& p)
    :AudioProcessorEditor(&p),processor(p) {
    setSize(900,620);setResizable(true,true);setResizeLimits(720,520,1200,850);
    for(auto* c:{&calibrateCard,&analyseCard,&translateCard})addAndMakeVisible(*c);
    calibrateCard.onClick=[this]{show(Page::Calibrate);};
    analyseCard.onClick=[this]{show(Page::Analyse);};
    translateCard.onClick=[this]{show(Page::Translate);};
    for(auto* b:{&back,&settings,&importButton,&analyseButton}) {
        addAndMakeVisible(*b);
        b->setColour(juce::TextButton::buttonColourId,juce::Colour(white));
        b->setColour(juce::TextButton::textColourOffId,juce::Colour(ink));
    }
    back.onClick=[this]{show(Page::Home);};
    settings.onClick=[this]{show(Page::Settings);};
    for(auto* l:{&profile,&readings,&advice,&note}) {
        addAndMakeVisible(*l);
        l->setJustificationType(juce::Justification::centred);
        l->setColour(juce::Label::textColourId,juce::Colour(ink));
        l->setFont(juce::Font(juce::FontOptions(17)));
    }
    for(auto* t:{&calibration,&simulation,&mix,&bypass}) {
        addAndMakeVisible(*t);
        t->setColour(juce::ToggleButton::textColourId,juce::Colour(ink));
        t->setColour(juce::ToggleButton::tickColourId,juce::Colour(0xff7f9d74));
    }
    addAndMakeVisible(device);
    device.setColour(juce::ComboBox::backgroundColourId,juce::Colour(white));
    device.setColour(juce::ComboBox::textColourId,juce::Colour(ink));
    device.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xffe2dbcf));
    const char* names[]={"Reference (off)","Phone","Android phone","Tablet","Laptop","TV","Car","Bluetooth","Earbuds","Consumer headphones","Studio monitors","Small mono","Limited playback"};
    for(int i=0;i<13;++i)device.addItem(names[i],i+1);
    for(auto* slider:{&intensity,&mixGain}) {
        addAndMakeVisible(*slider);
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,74,30);
        slider->setColour(juce::Slider::trackColourId,juce::Colour(0xff9db694));
        slider->setColour(juce::Slider::thumbColourId,juce::Colour(0xff6e8c64));
        slider->setColour(juce::Slider::textBoxTextColourId,juce::Colour(ink));
        slider->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    }
    auto& v=p.apvts;
    calAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(v,"cal",calibration);
    simAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(v,"sim",simulation);
    mixAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(v,"mix",mix);
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(v,"allbypass",bypass);
    deviceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(v,"device",device);
    intensityAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(v,"intensity",intensity);
    gainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(v,"mixgain",mixGain);
    importButton.onClick=[this] {
        auto chooser=std::make_shared<juce::FileChooser>("Import six-band correction CSV",juce::File{},"*.csv");
        chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this,chooser](const juce::FileChooser& c) {
            const auto file=c.getResult();if(file==juce::File{})return;
            juce::String error;
            if(!processor.importProfile(file,error))
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Profile not imported",error);
            profile.setText(processor.hasMeasuredProfile()?processor.getProfileName()+" · imported":"Not calibrated",juce::dontSendNotification);
        });
    };
    analyseButton.onClick=[this]{showingAdvice=true;timerCallback();};
    show(Page::Home);startTimerHz(30);
}
void UniversalReferenceAudioProcessorEditor::show(Page p) {
    page=p;
    const bool home=p==Page::Home,cal=p==Page::Calibrate,ana=p==Page::Analyse,trans=p==Page::Translate,advanced=p==Page::Settings;
    for(auto* c:{&calibrateCard,&analyseCard,&translateCard})c->setVisible(home);
    settings.setVisible(home);back.setVisible(!home);
    profile.setVisible(cal);importButton.setVisible(cal);calibration.setVisible(cal);intensity.setVisible(cal);
    readings.setVisible(ana);advice.setVisible(ana);analyseButton.setVisible(ana);
    device.setVisible(trans);simulation.setVisible(trans);
    mix.setVisible(advanced);mixGain.setVisible(advanced);bypass.setVisible(advanced);
    note.setVisible(!home);
    if(cal) {
        profile.setText(processor.hasMeasuredProfile()?processor.getProfileName()+" · imported":"Not calibrated",juce::dontSendNotification);
        note.setText("Import a measurement-derived correction CSV; imported does not mean verified.",juce::dontSendNotification);
    } else if(ana)note.setText("Basic peak and stereo checks · not LUFS or spectral analysis",juce::dontSendNotification);
    else if(trans)note.setText("Generic simulations, not exact physical devices",juce::dontSendNotification);
    else if(advanced)note.setText("Monitoring effects can print into exports. Bypass before bouncing.",juce::dontSendNotification);
    resized();repaint();
}
void UniversalReferenceAudioProcessorEditor::drawWave(juce::Graphics& g,juce::Rectangle<float> a,juce::Colour colour) {
    const auto level=juce::jlimit(.12f,1.0f,(displayLevel+62)/46);
    juce::Path wave;
    for(int i=0;i<=180;++i) {
        const auto x=i/180.f;
        const auto envelope=std::pow(std::sin(x*juce::MathConstants<float>::pi),1.4f);
        const auto shape=std::sin(x*51+phase)*.66f+std::sin(x*109-phase*.7f)*.3f;
        const auto y=a.getCentreY()-shape*envelope*a.getHeight()*.32f*level;
        if(i==0)wave.startNewSubPath(a.getX(),y);else wave.lineTo(a.getX()+x*a.getWidth(),y);
    }
    g.setColour(colour);
    g.strokePath(wave,juce::PathStrokeType(4,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void UniversalReferenceAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(cream));
    const float w=float(getWidth()),h=float(getHeight()),cx=w*.5f;
    caption(g,"universal.",{cx-150,22,300,46},31,juce::Colour(ink),true);
    if(page==Page::Home) {
        caption(g,"Your sound, everywhere.",{cx-280,79,560,30},20,juce::Colour(taupe));
        panel(g,{cx-185,136,370,143},juce::Colour(0xffede7db),35);
        drawWave(g,{cx-153,158,306,93},juce::Colour(0xff85a47c));
        caption(g,"Make it yours",{cx-170,h-72,340,27},16,juce::Colour(taupe));
    } else {
        const char* name=page==Page::Calibrate?"Calibrate":page==Page::Analyse?"Analyse":page==Page::Translate?"Translate":"Settings";
        caption(g,name,{cx-250,87,500,55},38,juce::Colour(ink),true);
        panel(g,{cx-315,157,630,h-241},juce::Colour(white),34);
        if(page==Page::Analyse)drawWave(g,{cx-251,180,502,102},juce::Colour(0xffd49d87));
        else {
            const juce::Colour c=page==Page::Calibrate?juce::Colour(0xffdfead9):page==Page::Translate?juce::Colour(0xffdee4f0):juce::Colour(0xffe8c8b8);
            g.setColour(c);g.fillRoundedRectangle(cx-53,184,106,106,32);
            caption(g,page==Page::Calibrate?"◉":page==Page::Translate?"◫":"✦",{cx-37,205,74,64},53,juce::Colour(ink));
        }
        caption(g,"Monitor carefully · export clean",{cx-260,h-58,520,23},13,juce::Colour(taupe));
    }
}
void UniversalReferenceAudioProcessorEditor::resized() {
    const int w=getWidth(),h=getHeight(),cx=w/2,gap=18;
    const int cw=juce::jmin(224,(w-105-2*gap)/3),left=(w-3*cw-2*gap)/2;
    settings.setBounds(w-65,23,42,41);back.setBounds(23,23,42,41);
    const int cardY=juce::jmin(308,h-252);
    calibrateCard.setBounds(left,cardY,cw,225);
    analyseCard.setBounds(left+cw+gap,cardY,cw,225);
    translateCard.setBounds(left+2*(cw+gap),cardY,cw,225);
    profile.setBounds(cx-250,306,500,32);importButton.setBounds(cx-132,351,264,45);
    calibration.setBounds(cx-128,421,256,38);intensity.setBounds(cx-166,468,332,42);
    readings.setBounds(cx-264,306,528,36);advice.setBounds(cx-260,358,520,64);
    analyseButton.setBounds(cx-120,441,240,50);
    device.setBounds(cx-211,329,422,49);simulation.setBounds(cx-120,407,240,45);
    mix.setBounds(cx-156,317,312,38);mixGain.setBounds(cx-165,364,330,42);
    bypass.setBounds(cx-156,422,312,39);
    note.setBounds(cx-310,h-123,620,32);
}
void UniversalReferenceAudioProcessorEditor::timerCallback() {
    phase+=.045f;
    displayLevel=.82f*displayLevel+.18f*processor.rmsDb.load();
    displayCorrelation=.82f*displayCorrelation+.18f*processor.correlation.load();
    readings.setText("Peak "+juce::String(processor.peakDb.load(),1)+" dBFS   ·   RMS "+juce::String(processor.rmsDb.load(),1)+" dBFS",juce::dontSendNotification);
    if(showingAdvice) {
        juce::String s="No strong peak or anti-phase warning. Keep checking by ear.";
        if(processor.peakDb.load()>-.15f)s="Peaks are near clipping. Leave some headroom.";
        else if(displayCorrelation<-.15f)s="Your stereo mix may cancel in mono. Check it.";
        advice.setText(s,juce::dontSendNotification);
    } else advice.setText("Tap below for a quick technical check",juce::dontSendNotification);
    if(page==Page::Home||page==Page::Analyse)repaint();
}
