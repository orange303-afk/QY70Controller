#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "QY70Midi.h"

#include <array>
#include <cmath>

namespace ids
{
constexpr auto part = "part";
constexpr auto bankMsb = "bankMsb";
constexpr auto bankLsb = "bankLsb";
constexpr auto program = "program";
constexpr auto volume = "volume";
constexpr auto pan = "pan";
constexpr auto cutoff = "cutoff";
constexpr auto resonance = "resonance";
constexpr auto attack = "attack";
constexpr auto release = "release";
constexpr auto chorus = "chorus";
constexpr auto reverb = "reverb";
constexpr auto variation = "variation";
} // namespace ids

namespace
{
constexpr std::array<const char*, 13> parameterIds {
    ids::part, ids::bankMsb, ids::bankLsb, ids::program,
    ids::volume, ids::pan, ids::cutoff, ids::resonance,
    ids::attack, ids::release, ids::chorus, ids::reverb, ids::variation
};
} // namespace

QY70ControllerAudioProcessor::QY70ControllerAudioProcessor()
    : AudioProcessor(BusesProperties()),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
    for (const auto* id : parameterIds)
        parameters.addParameterListener(id, this);
}

QY70ControllerAudioProcessor::~QY70ControllerAudioProcessor()
{
    for (const auto* id : parameterIds)
        parameters.removeParameterListener(id, this);
}

void QY70ControllerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    xgWaitSamples = 0;
}
void QY70ControllerAudioProcessor::releaseResources() {}

bool QY70ControllerAudioProcessor::isBusesLayoutSupported(const BusesLayout&) const
{
    return true;
}

void QY70ControllerAudioProcessor::processBlock(juce::AudioBuffer<float>& audio,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    remapInputToSelectedPart(midiMessages);
    emitPendingMessages(midiMessages, audio.getNumSamples());
}

juce::AudioProcessorEditor* QY70ControllerAudioProcessor::createEditor()
{
    return new QY70ControllerAudioProcessorEditor(*this);
}

void QY70ControllerAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destinationData);
}

void QY70ControllerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));

    dirtyMask.fetch_or(snapshotDirty, std::memory_order_release);
}

void QY70ControllerAudioProcessor::requestCurrentPart()
{
    dirtyMask.fetch_or(requestDirty, std::memory_order_release);
}

void QY70ControllerAudioProcessor::sendCurrentPartSnapshot()
{
    dirtyMask.fetch_or(snapshotDirty, std::memory_order_release);
}

void QY70ControllerAudioProcessor::enableXgMode()
{
    dirtyMask.fetch_or(xgSystemOnDirty, std::memory_order_release);
}

void QY70ControllerAudioProcessor::parameterChanged(const juce::String& parameterID, float)
{
    std::uint32_t bit = 0;

    if (parameterID == ids::part) bit = snapshotDirty;
    else if (parameterID == ids::bankMsb
             || parameterID == ids::bankLsb
             || parameterID == ids::program)
        bit = bankMsbDirty | bankLsbDirty | programDirty;
    else if (parameterID == ids::volume) bit = volumeDirty;
    else if (parameterID == ids::pan) bit = panDirty;
    else if (parameterID == ids::cutoff) bit = cutoffDirty;
    else if (parameterID == ids::resonance) bit = resonanceDirty;
    else if (parameterID == ids::attack) bit = attackDirty;
    else if (parameterID == ids::release) bit = releaseDirty;
    else if (parameterID == ids::chorus) bit = chorusDirty;
    else if (parameterID == ids::reverb) bit = reverbDirty;
    else if (parameterID == ids::variation) bit = variationDirty;

    dirtyMask.fetch_or(bit, std::memory_order_release);
}

int QY70ControllerAudioProcessor::parameterValue(const char* id) const
{
    if (const auto* value = parameters.getRawParameterValue(id))
        return juce::roundToInt(value->load());

    return 0;
}

void QY70ControllerAudioProcessor::remapInputToSelectedPart(juce::MidiBuffer& midiMessages) const
{
    juce::MidiBuffer remapped;
    const auto channel = juce::jlimit(1, 16, parameterValue(ids::part));

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.getChannel() > 0)
            message.setChannel(channel);

        remapped.addEvent(message, metadata.samplePosition);
    }

    midiMessages.swapWith(remapped);
}

void QY70ControllerAudioProcessor::emitPendingMessages(juce::MidiBuffer& midiMessages,
                                                        int blockSize)
{
    if (xgWaitSamples > 0)
    {
        xgWaitSamples = juce::jmax(0, xgWaitSamples - blockSize);
        if (xgWaitSamples > 0)
            return;
    }

    auto pending = dirtyMask.exchange(0, std::memory_order_acq_rel);
    if (pending == 0)
        return;

    if ((pending & xgSystemOnDirty) != 0)
    {
        midiMessages.addEvent(qy70::makeXgSystemOn(), 0);
        dirtyMask.fetch_or((pending & ~xgSystemOnDirty) | snapshotDirty,
                           std::memory_order_release);
        xgWaitSamples = juce::roundToInt(std::ceil(currentSampleRate * 0.06));
        return;
    }

    constexpr auto allParameterBits = bankMsbDirty | bankLsbDirty | programDirty
                                      | volumeDirty | panDirty | cutoffDirty
                                      | resonanceDirty | attackDirty | releaseDirty
                                      | chorusDirty | reverbDirty | variationDirty;

    if ((pending & snapshotDirty) != 0)
        pending |= allParameterBits;

    const auto part = parameterValue(ids::part);
    const auto add = [&midiMessages, part](std::uint32_t bit,
                                          std::uint32_t mask,
                                          qy70::MultiPartParameter parameter,
                                          int value)
    {
        if ((mask & bit) != 0)
            midiMessages.addEvent(qy70::makeMultiPartParameterChange(part, parameter, value), 0);
    };

    constexpr auto voiceSelectionBits = bankMsbDirty | bankLsbDirty | programDirty;
    if ((pending & voiceSelectionBits) != 0)
    {
        for (const auto& message : qy70::makeChannelVoiceSelection(part,
                                                                   parameterValue(ids::bankMsb),
                                                                   parameterValue(ids::bankLsb),
                                                                   parameterValue(ids::program)))
            midiMessages.addEvent(message, 0);
    }

    add(volumeDirty, pending, qy70::MultiPartParameter::volume, parameterValue(ids::volume));
    add(panDirty, pending, qy70::MultiPartParameter::pan, parameterValue(ids::pan));
    add(cutoffDirty, pending, qy70::MultiPartParameter::filterCutoff, parameterValue(ids::cutoff));
    add(resonanceDirty, pending, qy70::MultiPartParameter::filterResonance, parameterValue(ids::resonance));
    add(attackDirty, pending, qy70::MultiPartParameter::attackTime, parameterValue(ids::attack));
    add(releaseDirty, pending, qy70::MultiPartParameter::releaseTime, parameterValue(ids::release));
    add(chorusDirty, pending, qy70::MultiPartParameter::chorusSend, parameterValue(ids::chorus));
    add(reverbDirty, pending, qy70::MultiPartParameter::reverbSend, parameterValue(ids::reverb));
    add(variationDirty, pending, qy70::MultiPartParameter::variationSend, parameterValue(ids::variation));

    if ((pending & requestDirty) != 0)
    {
        const auto partIndex = static_cast<std::uint8_t>(juce::jlimit(1, 32, part) - 1);
        constexpr std::array<qy70::MultiPartParameter, 12> requestParameters {
            qy70::MultiPartParameter::bankMsb,
            qy70::MultiPartParameter::bankLsb,
            qy70::MultiPartParameter::program,
            qy70::MultiPartParameter::volume,
            qy70::MultiPartParameter::pan,
            qy70::MultiPartParameter::filterCutoff,
            qy70::MultiPartParameter::filterResonance,
            qy70::MultiPartParameter::attackTime,
            qy70::MultiPartParameter::releaseTime,
            qy70::MultiPartParameter::chorusSend,
            qy70::MultiPartParameter::reverbSend,
            qy70::MultiPartParameter::variationSend
        };

        for (const auto parameter : requestParameters)
        {
            midiMessages.addEvent(qy70::makeXgParameterRequest(
                                      0,
                                      { 0x08, partIndex, static_cast<std::uint8_t>(parameter) }),
                                  0);
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
QY70ControllerAudioProcessor::createParameterLayout()
{
    using IntParameter = juce::AudioParameterInt;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<IntParameter>(ids::part, "Part", 1, 16, 1));
    layout.add(std::make_unique<IntParameter>(ids::bankMsb, "Bank MSB", 0, 127, 0));
    layout.add(std::make_unique<IntParameter>(ids::bankLsb, "Bank LSB", 0, 127, 0));
    layout.add(std::make_unique<IntParameter>(ids::program, "Patch", 1, 128, 1));
    layout.add(std::make_unique<IntParameter>(ids::volume, "Volume", 0, 127, 100));
    layout.add(std::make_unique<IntParameter>(ids::pan, "Pan", 0, 127, 64));
    layout.add(std::make_unique<IntParameter>(ids::cutoff, "Cutoff", 0, 127, 64));
    layout.add(std::make_unique<IntParameter>(ids::resonance, "Resonance", 0, 127, 64));
    layout.add(std::make_unique<IntParameter>(ids::attack, "Attack", 0, 127, 64));
    layout.add(std::make_unique<IntParameter>(ids::release, "Release", 0, 127, 64));
    layout.add(std::make_unique<IntParameter>(ids::chorus, "Chorus Send", 0, 127, 0));
    layout.add(std::make_unique<IntParameter>(ids::reverb, "Reverb Send", 0, 127, 40));
    layout.add(std::make_unique<IntParameter>(ids::variation, "Variation Send", 0, 127, 0));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QY70ControllerAudioProcessor();
}
