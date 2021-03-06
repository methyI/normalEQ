/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NormalEQAudioProcessor::NormalEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

NormalEQAudioProcessor::~NormalEQAudioProcessor()
{
}

//==============================================================================
const juce::String NormalEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NormalEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NormalEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NormalEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NormalEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NormalEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NormalEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NormalEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NormalEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void NormalEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NormalEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // ProcessSpec ???????????? ?????????
    // This structure is passed into a DSP algorithm's prepare() method, and contains
    // information about various aspects of the context in which it can expect to be called.
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    

    updateFilters();
    
}

void NormalEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NormalEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NormalEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // dsp::ProcessorChains  dsp::ProcessContextReplacing<>
    // ???????????? ??????????????? ????????? ????????? ?????? ???????????? ???????????? ?????? ???????????? ??????????????? ?????????
    // ???????????? ??????????????? ????????? ???????????? ????????? ????????? ?????? ???????????? ???
    
    // dsp::AudioBlock<> ?????????????????? juce::AudioBuffer<>??? ?????? ???????????????.
    // ???????????? ?????? ????????? ???????????? ?????? ????????????, ????????? ?????? ?????? ?????? ????????? ????????????.
    // ????????? ??? ????????? ?????? ???, ??? ????????? ????????????. channel {0, 1}
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateFilters();
    
    
    juce::dsp::AudioBlock<float> block(buffer); // ?????? ????????? ????????? ????????? ???
    
    // audioBlock<> ??????????????? ??? ?????? ????????? ?????? ??????????????? ?????? ????????? ????????? ??? ?????? ????????? ??????
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    // ?????? ??? ?????? ?????? ????????? ???????????? ????????? ???????????? ??????????????? ????????? ??? = ???????????? ?????? ???????????????
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool NormalEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NormalEQAudioProcessor::createEditor()
{
    return new NormalEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void NormalEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // MemoryOutputStream?????? ?????????
    // Writes data to an internal memory buffer, which grows as required.
    // The data that was written into the stream can then be accessed later as a contiguous block of memory.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void NormalEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // ????????? ????????? ??????????????? ???????????? ????????? ????????? ??? ??????
    
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if( tree.isValid() )
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    // thread??? ?????? ??? ????????? ??????????????? ????????? ????????? ??????
    // apvts.getParameter("LowCut Freq")->getValue(); ??? ?????? ???????????? ?????? ???????????? ????????? ?????? x
    
    // ?????? ???????????? ?????????????????? ???????????? ???????????? ?????? ?????? ??? ?????? ???
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    
    // Slope??? ????????? ?????????
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    
    return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peakFreq,
                                                               chainSettings.peakQuality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void NormalEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    // ????????? ????????? ??????
    // auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
    //                                                                            chainSettings.peakFreq,
    //                                                                            chainSettings.peakQuality,
    //                                                                           juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    //
    // chain get ????????? ????????? ????????? ???????????? ????????? ??????.
    // enum??? ?????? ???????????? ???????????? ???????????????, ??????????????? ??????(????????? ?????? ?????? ???????????? ?????? ?????? ?????? ??????????????????)
    // *leftChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
    // *rightChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
    
    // ????????????
    // update filter > make filter > update coefficients > update filter
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    
    updateCoefficients(leftChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    //
    *old = *replacements;
}

void NormalEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPosition::LowCut>();
    updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void NormalEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
    auto cutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    
    auto& leftHighCut = leftChain.get<ChainPosition::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPosition::HighCut>();
    updateCutFilter(leftHighCut, cutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, cutCoefficients, chainSettings.highCutSlope);
}

void NormalEQAudioProcessor::updateFilters()
{
    // ?????? ???????????? ??????????????? ??? ?????? ????????? ???????????????
    auto chainSettings = getChainSettings(apvts);
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout NormalEQAudioProcessor::createParameterLayout()
{
    // AudioProcessorParameter Diagram ??????
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.6f),
                                                           20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.6f),
                                                           20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.3f),
                                                           1000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 0.2f),
                                                           0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 0.3f),
                                                           1.f));
    
    juce::StringArray stringArray;
    for(int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope",
                                                            "LowCut Slope",
                                                            stringArray,
                                                            0));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope",
                                                            "HighCut Slope",
                                                            stringArray,
                                                            0));
    
    
    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NormalEQAudioProcessor();
}
