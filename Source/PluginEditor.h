/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AbletonStyleBox.h"


struct CustomColour
{
    const juce::Colour background   = juce::Colour::fromRGB(12, 22, 49);
    const juce::Colour almondAlpha  = juce::Colour::fromRGBA(236, 216, 200, 60);
    const juce::Colour almond       = juce::Colour::fromRGB(236, 216, 200);
    const juce::Colour zest         = juce::Colour::fromRGB(218, 121, 25);
    const juce::Colour mahogany     = juce::Colour::fromRGB(97, 8, 7);
};

struct DrawResponseCurve : juce::Component,
                           juce::AudioProcessorParameter::Listener,
                           juce::Timer
{
    DrawResponseCurve(NormalEQAudioProcessor& p);
    ~DrawResponseCurve();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateChain();

private:
    NormalEQAudioProcessor& audioProcessor;
    CustomColour customColour;
    juce::Atomic<bool> parameterChanged{ false };
    MonoChain  monoChain;
    
    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();
};


struct CustomDialLookAndFeel : public juce::LookAndFeel_V4
{
    CustomDialLookAndFeel();
    ~CustomDialLookAndFeel();
    
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;
    void drawRotarySlider (juce::Graphics&, int x, int y,
                           int width, int height, float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    
    juce::Label* createSliderTextBox (juce::Slider& slider) override;
    
private:
    CustomColour customColour;
};


struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider();
    ~CustomRotarySlider();

private:
    CustomColour customColour;
    CustomDialLookAndFeel customDialLookAndFeel;
};


struct DrawImage : public juce::Component
{
    
    const std::unique_ptr<juce::Drawable> lowCut = juce::Drawable::createFromImageData(BinaryData::highpass_svg, BinaryData::highpass_svgSize);
    const std::unique_ptr<juce::Drawable> peak = juce::Drawable::createFromImageData(BinaryData::bell_svg, BinaryData::bell_svgSize);
    const std::unique_ptr<juce::Drawable> highCut = juce::Drawable::createFromImageData(BinaryData::lowpass_svg, BinaryData::lowpass_svgSize);
    
};

//==============================================================================
/**
*/

// ???????????? ????????? ???????????? ?????? ?????? ????????? ???????????? ???????????? ???
// ???????????? ????????? ????????? ??????????????? ????????????. ?????? ????????? ???????????? editor??? filterChain??? ?????????????????? ?????? ?????? \
// gui ????????? ??? ??? ????????? ???
// ????????? atomic flag??? ?????? ???????????? ????????? ??? ?????????, ?????? ???????????? ??????????????? ??? ??????.

class NormalEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    NormalEQAudioProcessorEditor (NormalEQAudioProcessor&);
    ~NormalEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
        
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    NormalEQAudioProcessor& audioProcessor;
    CustomColour customColour;
    DrawImage drawImage;
    CustomLookAndFeel customLookAndFeel;
    
    AbletonStyleBox highCutFreqBox,
                    peakFreqBox,
                    peakGainBox,
                    peakQualityBox,
                    lowCutFreqBox;
    
    CustomRotarySlider highCutSlopeSlider,
                       lowCutSlopeSlider;
    
    DrawResponseCurve drawResponseCurveComponent;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attatchment = APVTS::SliderAttachment;
    
    Attatchment highCutFreqBoxAttatchment,
                peakFreqBoxAttatchment,
                peakGainBoxAttatchment,
                peakQualityBoxAttatchment,
                lowCutFreqBoxAttatchment,
                highCutSlopeSliderAttatchment,
                lowCutSlopeSliderAttatchment;
    
    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NormalEQAudioProcessorEditor)
};
