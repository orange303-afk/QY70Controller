#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "QY70Midi.h"

#include <array>
#include <cmath>
#include <vector>

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
constexpr auto monoPoly = "monoPoly";
constexpr auto keyAssign = "keyAssign";
constexpr auto partMode = "partMode";
constexpr auto noteShift = "noteShift";
constexpr auto detune = "detune";
constexpr auto velocityDepth = "velocityDepth";
constexpr auto velocityOffset = "velocityOffset";
constexpr auto noteLimitLow = "noteLimitLow";
constexpr auto noteLimitHigh = "noteLimitHigh";
constexpr auto dryLevel = "dryLevel";
constexpr auto vibratoRate = "vibratoRate";
constexpr auto vibratoDepth = "vibratoDepth";
constexpr auto vibratoDelay = "vibratoDelay";
constexpr auto decay = "decay";
constexpr auto mwPitch = "mwPitch";
constexpr auto mwFilter = "mwFilter";
constexpr auto mwAmplitude = "mwAmplitude";
constexpr auto mwLfoPitch = "mwLfoPitch";
constexpr auto mwLfoFilter = "mwLfoFilter";
constexpr auto mwLfoAmplitude = "mwLfoAmplitude";
constexpr auto bendPitch = "bendPitch";
constexpr auto bendFilter = "bendFilter";
constexpr auto bendAmplitude = "bendAmplitude";
constexpr auto bendLfoPitch = "bendLfoPitch";
constexpr auto bendLfoFilter = "bendLfoFilter";
constexpr auto bendLfoAmplitude = "bendLfoAmplitude";
constexpr auto aftertouchPitch = "aftertouchPitch";
constexpr auto aftertouchFilter = "aftertouchFilter";
constexpr auto aftertouchAmplitude = "aftertouchAmplitude";
constexpr auto aftertouchLfoPitch = "aftertouchLfoPitch";
constexpr auto aftertouchLfoFilter = "aftertouchLfoFilter";
constexpr auto aftertouchLfoAmplitude = "aftertouchLfoAmplitude";
constexpr auto portamentoSwitch = "portamentoSwitch";
constexpr auto portamentoTime = "portamentoTime";
constexpr auto pitchEgInitial = "pitchEgInitial";
constexpr auto pitchEgAttack = "pitchEgAttack";
constexpr auto pitchEgReleaseLevel = "pitchEgReleaseLevel";
constexpr auto pitchEgReleaseTime = "pitchEgReleaseTime";
} // namespace ids

namespace
{
constexpr std::array<const char*, 13> coreParameterIds {
    ids::part, ids::bankMsb, ids::bankLsb, ids::program,
    ids::volume, ids::pan, ids::cutoff, ids::resonance,
    ids::attack, ids::release, ids::chorus, ids::reverb, ids::variation
};

enum class PartEncoding { direct, signedAround64, detuneTenthsHz };

struct PartBinding
{
    const char* id;
    qy70::MultiPartParameter parameter;
    PartEncoding encoding;
};

constexpr std::array<PartBinding, 38> advancedPartBindings {{
    { ids::monoPoly, qy70::MultiPartParameter::monoPolyMode, PartEncoding::direct },
    { ids::keyAssign, qy70::MultiPartParameter::sameNoteKeyAssign, PartEncoding::direct },
    { ids::partMode, qy70::MultiPartParameter::partMode, PartEncoding::direct },
    { ids::noteShift, qy70::MultiPartParameter::noteShift, PartEncoding::signedAround64 },
    { ids::detune, qy70::MultiPartParameter::detune, PartEncoding::detuneTenthsHz },
    { ids::velocityDepth, qy70::MultiPartParameter::velocitySenseDepth, PartEncoding::direct },
    { ids::velocityOffset, qy70::MultiPartParameter::velocitySenseOffset, PartEncoding::direct },
    { ids::noteLimitLow, qy70::MultiPartParameter::noteLimitLow, PartEncoding::direct },
    { ids::noteLimitHigh, qy70::MultiPartParameter::noteLimitHigh, PartEncoding::direct },
    { ids::dryLevel, qy70::MultiPartParameter::dryLevel, PartEncoding::direct },
    { ids::vibratoRate, qy70::MultiPartParameter::vibratoRate, PartEncoding::signedAround64 },
    { ids::vibratoDepth, qy70::MultiPartParameter::vibratoDepth, PartEncoding::signedAround64 },
    { ids::vibratoDelay, qy70::MultiPartParameter::vibratoDelay, PartEncoding::signedAround64 },
    { ids::decay, qy70::MultiPartParameter::decayTime, PartEncoding::signedAround64 },
    { ids::mwPitch, qy70::MultiPartParameter::modulationWheelPitch, PartEncoding::signedAround64 },
    { ids::mwFilter, qy70::MultiPartParameter::modulationWheelFilter, PartEncoding::signedAround64 },
    { ids::mwAmplitude, qy70::MultiPartParameter::modulationWheelAmplitude, PartEncoding::signedAround64 },
    { ids::mwLfoPitch, qy70::MultiPartParameter::modulationWheelLfoPitchDepth, PartEncoding::direct },
    { ids::mwLfoFilter, qy70::MultiPartParameter::modulationWheelLfoFilterDepth, PartEncoding::direct },
    { ids::mwLfoAmplitude, qy70::MultiPartParameter::modulationWheelLfoAmplitudeDepth, PartEncoding::direct },
    { ids::bendPitch, qy70::MultiPartParameter::pitchBendPitch, PartEncoding::signedAround64 },
    { ids::bendFilter, qy70::MultiPartParameter::pitchBendFilter, PartEncoding::signedAround64 },
    { ids::bendAmplitude, qy70::MultiPartParameter::pitchBendAmplitude, PartEncoding::signedAround64 },
    { ids::bendLfoPitch, qy70::MultiPartParameter::pitchBendLfoPitchDepth, PartEncoding::direct },
    { ids::bendLfoFilter, qy70::MultiPartParameter::pitchBendLfoFilterDepth, PartEncoding::direct },
    { ids::bendLfoAmplitude, qy70::MultiPartParameter::pitchBendLfoAmplitudeDepth, PartEncoding::direct },
    { ids::aftertouchPitch, qy70::MultiPartParameter::aftertouchPitch, PartEncoding::signedAround64 },
    { ids::aftertouchFilter, qy70::MultiPartParameter::aftertouchFilter, PartEncoding::signedAround64 },
    { ids::aftertouchAmplitude, qy70::MultiPartParameter::aftertouchAmplitude, PartEncoding::signedAround64 },
    { ids::aftertouchLfoPitch, qy70::MultiPartParameter::aftertouchLfoPitchDepth, PartEncoding::direct },
    { ids::aftertouchLfoFilter, qy70::MultiPartParameter::aftertouchLfoFilterDepth, PartEncoding::direct },
    { ids::aftertouchLfoAmplitude, qy70::MultiPartParameter::aftertouchLfoAmplitudeDepth, PartEncoding::direct },
    { ids::portamentoSwitch, qy70::MultiPartParameter::portamentoSwitch, PartEncoding::direct },
    { ids::portamentoTime, qy70::MultiPartParameter::portamentoTime, PartEncoding::direct },
    { ids::pitchEgInitial, qy70::MultiPartParameter::pitchEgInitialLevel, PartEncoding::signedAround64 },
    { ids::pitchEgAttack, qy70::MultiPartParameter::pitchEgAttackTime, PartEncoding::signedAround64 },
    { ids::pitchEgReleaseLevel, qy70::MultiPartParameter::pitchEgReleaseLevel, PartEncoding::signedAround64 },
    { ids::pitchEgReleaseTime, qy70::MultiPartParameter::pitchEgReleaseTime, PartEncoding::signedAround64 }
}};

enum class EffectEncoding { direct, fourteenBit, type, variationPart };

struct EffectBinding
{
    juce::String id, name;
    std::uint8_t address;
    int minimum, maximum, defaultValue;
    EffectEncoding encoding;
    qy70::EffectBlock block;
};

const std::vector<EffectBinding>& effectBindings()
{
    static const auto bindings = []
    {
        std::vector<EffectBinding> result;
        const auto addType = [&result](const char* id, const char* name,
                                       std::uint8_t address, qy70::EffectBlock block,
                                       int defaultIndex)
        {
            result.push_back({ id, name, address, 0,
                               static_cast<int>(qy70::effectTypes(block).size()) - 1,
                               defaultIndex, EffectEncoding::type, block });
        };
        const auto addParameters = [&result](const char* prefix, const char* label,
                                             std::uint8_t firstAddress,
                                             std::uint8_t optionAddress,
                                             qy70::EffectBlock block,
                                             bool fourteenBit)
        {
            for (int number = 1; number <= 16; ++number)
            {
                const auto address = number <= 10
                                         ? static_cast<std::uint8_t>(firstAddress
                                             + (fourteenBit ? (number - 1) * 2 : number - 1))
                                         : static_cast<std::uint8_t>(optionAddress + number - 11);
                result.push_back({ juce::String(prefix) + juce::String(number),
                                   juce::String(label) + " Param " + juce::String(number),
                                   address, 0,
                                   fourteenBit && number <= 10 ? 16383 : 127,
                                   0,
                                   fourteenBit && number <= 10 ? EffectEncoding::fourteenBit
                                                               : EffectEncoding::direct,
                                   block });
            }
        };

        addType("reverbType", "Reverb Type", 0x00, qy70::EffectBlock::reverb, 1);
        addParameters("reverbParam", "Reverb", 0x02, 0x10, qy70::EffectBlock::reverb, false);
        result.push_back({ "reverbReturn", "Reverb Return", 0x0C, 0, 127, 96,
                           EffectEncoding::direct, qy70::EffectBlock::reverb });
        result.push_back({ "reverbPan", "Reverb Pan", 0x0D, 1, 127, 64,
                           EffectEncoding::direct, qy70::EffectBlock::reverb });

        addType("chorusType", "Chorus Type", 0x20, qy70::EffectBlock::chorus, 1);
        addParameters("chorusParam", "Chorus", 0x22, 0x30, qy70::EffectBlock::chorus, false);
        result.push_back({ "chorusReturn", "Chorus Return", 0x2C, 0, 127, 96,
                           EffectEncoding::direct, qy70::EffectBlock::chorus });
        result.push_back({ "chorusPan", "Chorus Pan", 0x2D, 1, 127, 64,
                           EffectEncoding::direct, qy70::EffectBlock::chorus });
        result.push_back({ "chorusToReverb", "Chorus To Reverb", 0x2E, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::chorus });

        addType("variationType", "Variation / Delay Type", 0x40,
                qy70::EffectBlock::variation, 9);
        addParameters("variationParam", "Variation", 0x42, 0x70,
                      qy70::EffectBlock::variation, true);
        result.push_back({ "variationReturn", "Variation Return", 0x56, 0, 127, 96,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "variationPan", "Variation Pan", 0x57, 1, 127, 64,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "variationToReverb", "Variation To Reverb", 0x58, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "variationToChorus", "Variation To Chorus", 0x59, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "variationConnection", "Variation Connection", 0x5A, 0, 1, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "variationPart", "Variation Insertion Part", 0x5B, 0, 32, 32,
                           EffectEncoding::variationPart, qy70::EffectBlock::variation });
        result.push_back({ "mwVariationDepth", "MW Variation Depth", 0x5C, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "pbVariationDepth", "PB Variation Depth", 0x5D, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "atVariationDepth", "Aftertouch Variation Depth", 0x5E, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "ac1VariationDepth", "AC1 Variation Depth", 0x5F, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        result.push_back({ "ac2VariationDepth", "AC2 Variation Depth", 0x60, 0, 127, 0,
                           EffectEncoding::direct, qy70::EffectBlock::variation });
        return result;
    }();
    return bindings;
}

const std::vector<juce::String>& allParameterIds()
{
    static const auto result = []
    {
        std::vector<juce::String> values;
        for (const auto* id : coreParameterIds) values.emplace_back(id);
        for (const auto& binding : advancedPartBindings) values.emplace_back(binding.id);
        for (const auto& binding : effectBindings()) values.push_back(binding.id);
        return values;
    }();
    return result;
}

std::uint64_t maskForCount(std::size_t count)
{
    return count >= 64 ? ~std::uint64_t { 0 }
                       : (std::uint64_t { 1 } << count) - 1;
}

} // namespace

QY70ControllerAudioProcessor::QY70ControllerAudioProcessor()
    : AudioProcessor(BusesProperties()),
      parameters(*this, nullptr, "STATE", createParameterLayout())
{
    for (auto& mask : effectDirtyMasks)
        mask.store(0, std::memory_order_relaxed);
    for (const auto& id : allParameterIds()) parameters.addParameterListener(id, this);
}

QY70ControllerAudioProcessor::~QY70ControllerAudioProcessor()
{
    for (const auto& id : allParameterIds()) parameters.removeParameterListener(id, this);
}

void QY70ControllerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    xgWaitSamples = 0;
}

void QY70ControllerAudioProcessor::releaseResources() {}

bool QY70ControllerAudioProcessor::isBusesLayoutSupported(const BusesLayout&) const { return true; }

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
    effectSnapshotDirty.store(true, std::memory_order_release);
}

void QY70ControllerAudioProcessor::requestCurrentPart()
{
    dirtyMask.fetch_or(requestDirty, std::memory_order_release);
}

void QY70ControllerAudioProcessor::sendCurrentPartSnapshot()
{
    dirtyMask.fetch_or(snapshotDirty, std::memory_order_release);
    effectSnapshotDirty.store(true, std::memory_order_release);
}

void QY70ControllerAudioProcessor::resetEditingParameters()
{
    for (const auto& id : allParameterIds())
    {
        if (id == ids::part || id == ids::bankMsb
            || id == ids::bankLsb || id == ids::program)
            continue;

        if (auto* parameter = parameters.getParameter(id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
            parameter->endChangeGesture();
        }
    }

    effectDirtyMasks[0].store(0, std::memory_order_release);
    effectDirtyMasks[1].store(0, std::memory_order_release);
    effectSnapshotDirty.store(false, std::memory_order_release);
    dirtyMask.fetch_or(snapshotDirty, std::memory_order_release);
    setXgModeEnabled(true);
}

void QY70ControllerAudioProcessor::setXgModeEnabled(bool shouldBeEnabled)
{
    xgModeEnabled.store(shouldBeEnabled, std::memory_order_release);
    dirtyMask.fetch_or(shouldBeEnabled ? xgSystemOnDirty : gmSystemOnDirty,
                       std::memory_order_release);
}

void QY70ControllerAudioProcessor::parameterChanged(const juce::String& parameterID, float)
{
    std::uint32_t bit = 0;
    if (parameterID == ids::part) bit = snapshotDirty;
    else if (parameterID == ids::bankMsb || parameterID == ids::bankLsb
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

    if (bit != 0)
    {
        dirtyMask.fetch_or(bit, std::memory_order_release);
        return;
    }
    for (std::size_t index = 0; index < advancedPartBindings.size(); ++index)
        if (parameterID == advancedPartBindings[index].id)
        {
            advancedPartDirtyMask.fetch_or(std::uint64_t { 1 } << index,
                                           std::memory_order_release);
            return;
        }
    const auto& effects = effectBindings();
    for (std::size_t index = 0; index < effects.size(); ++index)
        if (parameterID == effects[index].id)
        {
            effectDirtyMasks[index / 64].fetch_or(std::uint64_t { 1 } << (index % 64),
                                                  std::memory_order_release);
            return;
        }
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
        if (message.getChannel() > 0) message.setChannel(channel);
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
        if (xgWaitSamples > 0) return;
    }

    auto pending = dirtyMask.exchange(0, std::memory_order_acq_rel);
    auto partPending = advancedPartDirtyMask.exchange(0, std::memory_order_acq_rel);
    std::array<std::uint64_t, 2> effectPending {
        effectDirtyMasks[0].exchange(0, std::memory_order_acq_rel),
        effectDirtyMasks[1].exchange(0, std::memory_order_acq_rel)
    };
    const auto sendEffectSnapshot = effectSnapshotDirty.exchange(false,
                                                                 std::memory_order_acq_rel);
    if (pending == 0 && partPending == 0 && effectPending[0] == 0
        && effectPending[1] == 0 && !sendEffectSnapshot)
        return;

    constexpr auto systemModeBits = xgSystemOnDirty | gmSystemOnDirty;
    if ((pending & systemModeBits) != 0)
    {
        midiMessages.addEvent(isXgModeEnabled() ? qy70::makeXgSystemOn()
                                                : qy70::makeGeneralMidiSystemOn(), 0);
        dirtyMask.fetch_or((pending & ~systemModeBits) | snapshotDirty,
                           std::memory_order_release);
        advancedPartDirtyMask.fetch_or(partPending, std::memory_order_release);
        effectDirtyMasks[0].fetch_or(effectPending[0], std::memory_order_release);
        effectDirtyMasks[1].fetch_or(effectPending[1], std::memory_order_release);
        if (sendEffectSnapshot)
            effectSnapshotDirty.store(true, std::memory_order_release);
        xgWaitSamples = juce::roundToInt(std::ceil(currentSampleRate * 0.06));
        return;
    }

    constexpr auto allCoreBits = bankMsbDirty | bankLsbDirty | programDirty
                                 | volumeDirty | panDirty | cutoffDirty | resonanceDirty
                                 | attackDirty | releaseDirty | chorusDirty | reverbDirty
                                 | variationDirty;
    if ((pending & snapshotDirty) != 0)
    {
        pending |= allCoreBits;
        partPending |= maskForCount(advancedPartBindings.size());
    }
    if (sendEffectSnapshot)
    {
        const auto effectCount = effectBindings().size();
        effectPending[0] |= maskForCount(juce::jmin<std::size_t>(64, effectCount));
        if (effectCount > 64) effectPending[1] |= maskForCount(effectCount - 64);
    }

    const auto part = parameterValue(ids::part);
    const auto add = [&midiMessages, part](std::uint32_t testBit, std::uint32_t mask,
                                          qy70::MultiPartParameter parameter, int value)
    {
        if ((mask & testBit) != 0)
            midiMessages.addEvent(qy70::makeMultiPartParameterChange(part, parameter, value), 0);
    };

    constexpr auto voiceSelectionBits = bankMsbDirty | bankLsbDirty | programDirty;
    if ((pending & voiceSelectionBits) != 0)
    {
        auto sampleOffset = 0;
        const auto lastSample = juce::jmax(0, blockSize - 1);
        for (const auto& message : qy70::makeChannelVoiceSelection(
                 part, parameterValue(ids::bankMsb), parameterValue(ids::bankLsb),
                 parameterValue(ids::program)))
            midiMessages.addEvent(message, juce::jmin(sampleOffset++, lastSample));
        const auto addVoice = [&](qy70::MultiPartParameter parameter, int value)
        {
            midiMessages.addEvent(qy70::makeMultiPartParameterChange(part, parameter, value),
                                  juce::jmin(sampleOffset++, lastSample));
        };
        addVoice(qy70::MultiPartParameter::bankMsb, parameterValue(ids::bankMsb));
        addVoice(qy70::MultiPartParameter::bankLsb, parameterValue(ids::bankLsb));
        addVoice(qy70::MultiPartParameter::program, parameterValue(ids::program) - 1);
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

    for (std::size_t index = 0; index < advancedPartBindings.size(); ++index)
    {
        if ((partPending & (std::uint64_t { 1 } << index)) == 0) continue;
        const auto& binding = advancedPartBindings[index];
        const auto value = parameterValue(binding.id);
        if (binding.encoding == PartEncoding::detuneTenthsHz)
            midiMessages.addEvent(qy70::makeMultiPartParameterChange(
                                      part, binding.parameter, qy70::encodeDetuneTenthsHz(value)), 0);
        else
            midiMessages.addEvent(qy70::makeMultiPartParameterChange(
                                      part, binding.parameter,
                                      binding.encoding == PartEncoding::signedAround64
                                          ? value + 64 : value), 0);
    }

    const auto& effects = effectBindings();
    for (std::size_t index = 0; index < effects.size(); ++index)
    {
        if ((effectPending[index / 64] & (std::uint64_t { 1 } << (index % 64))) == 0)
            continue;
        const auto& binding = effects[index];
        const auto value = parameterValue(binding.id.toRawUTF8());
        std::vector<std::uint8_t> data;
        switch (binding.encoding)
        {
            case EffectEncoding::direct:
                data = { static_cast<std::uint8_t>(juce::jlimit(0, 127, value)) };
                break;
            case EffectEncoding::fourteenBit: data = qy70::encode14Bit(value); break;
            case EffectEncoding::variationPart:
                data = { static_cast<std::uint8_t>(value >= 32 ? 127 : value) };
                break;
            case EffectEncoding::type:
            {
                const auto& types = qy70::effectTypes(binding.block);
                const auto& type = types[static_cast<std::size_t>(
                    juce::jlimit(0, static_cast<int>(types.size()) - 1, value))];
                data = { type.msb, type.lsb };
                break;
            }
        }
        midiMessages.addEvent(qy70::makeEffectParameterChange(binding.address, data), 0);
    }

    if ((pending & requestDirty) != 0)
    {
        const auto partIndex = static_cast<std::uint8_t>(juce::jlimit(1, 32, part) - 1);
        constexpr std::array<qy70::MultiPartParameter, 12> coreRequests {
            qy70::MultiPartParameter::bankMsb, qy70::MultiPartParameter::bankLsb,
            qy70::MultiPartParameter::program, qy70::MultiPartParameter::volume,
            qy70::MultiPartParameter::pan, qy70::MultiPartParameter::filterCutoff,
            qy70::MultiPartParameter::filterResonance, qy70::MultiPartParameter::attackTime,
            qy70::MultiPartParameter::releaseTime, qy70::MultiPartParameter::chorusSend,
            qy70::MultiPartParameter::reverbSend, qy70::MultiPartParameter::variationSend
        };
        for (const auto parameter : coreRequests)
            midiMessages.addEvent(qy70::makeXgParameterRequest(
                                      0, { 0x08, partIndex, static_cast<std::uint8_t>(parameter) }), 0);
        for (const auto& binding : advancedPartBindings)
            midiMessages.addEvent(qy70::makeXgParameterRequest(
                                      0, { 0x08, partIndex,
                                           static_cast<std::uint8_t>(binding.parameter) }), 0);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
QY70ControllerAudioProcessor::createParameterLayout()
{
    using IntParameter = juce::AudioParameterInt;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto addInt = [&layout](const char* id, const char* name,
                                  int min, int max, int defaultValue)
    {
        layout.add(std::make_unique<IntParameter>(id, name, min, max, defaultValue));
    };
    const auto addChoice = [&layout](const char* id, const char* name,
                                     const juce::StringArray& choices, int defaultIndex)
    {
        layout.add(std::make_unique<juce::AudioParameterChoice>(id, name, choices, defaultIndex));
    };

    addInt(ids::part, "Part", 1, 16, 1);
    addInt(ids::bankMsb, "Voice Mode", 0, 127, 0);
    addInt(ids::bankLsb, "XG Variation", 0, 127, 0);
    addInt(ids::program, "Patch", 1, 128, 1);
    addInt(ids::volume, "Volume", 0, 127, 100);
    addInt(ids::pan, "Pan", 0, 127, 64);
    addInt(ids::cutoff, "Cutoff", 0, 127, 64);
    addInt(ids::resonance, "Resonance", 0, 127, 64);
    addInt(ids::attack, "Attack", 0, 127, 64);
    addInt(ids::release, "Release", 0, 127, 64);
    addInt(ids::chorus, "Chorus Send", 0, 127, 0);
    addInt(ids::reverb, "Reverb Send", 0, 127, 40);
    addInt(ids::variation, "Variation Send", 0, 127, 0);
    addChoice(ids::monoPoly, "Mono / Poly", { "Mono", "Poly" }, 1);
    addChoice(ids::keyAssign, "Same Note Assign", { "Single", "Multi", "Instrument" }, 1);
    addChoice(ids::partMode, "Part Mode", { "Normal", "Drum Thru", "Drum 1", "Drum 2" }, 0);
    addInt(ids::noteShift, "Note Shift", -24, 24, 0);
    addInt(ids::detune, "Detune (0.1 Hz)", -128, 127, 0);
    addInt(ids::velocityDepth, "Velocity Depth", 0, 127, 64);
    addInt(ids::velocityOffset, "Velocity Offset", 0, 127, 64);
    addInt(ids::noteLimitLow, "Note Limit Low", 0, 127, 0);
    addInt(ids::noteLimitHigh, "Note Limit High", 0, 127, 127);
    addInt(ids::dryLevel, "Dry Level", 0, 127, 127);
    addInt(ids::vibratoRate, "Vibrato Rate", -64, 63, 0);
    addInt(ids::vibratoDepth, "Vibrato Depth", -64, 63, 0);
    addInt(ids::vibratoDelay, "Vibrato Delay", -64, 63, 0);
    addInt(ids::decay, "EG Decay", -64, 63, 0);
    addInt(ids::mwPitch, "MW Pitch", -24, 24, 0);
    addInt(ids::mwFilter, "MW Filter", -64, 63, 0);
    addInt(ids::mwAmplitude, "MW Amplitude", -64, 63, 0);
    addInt(ids::mwLfoPitch, "MW LFO Pitch Depth", 0, 127, 10);
    addInt(ids::mwLfoFilter, "MW LFO Filter Depth", 0, 127, 0);
    addInt(ids::mwLfoAmplitude, "MW LFO Amp Depth", 0, 127, 0);
    addInt(ids::bendPitch, "Pitch Bend Range", -24, 24, 2);
    addInt(ids::bendFilter, "Pitch Bend Filter", -64, 63, 0);
    addInt(ids::bendAmplitude, "Pitch Bend Amplitude", -64, 63, 0);
    addInt(ids::bendLfoPitch, "Pitch Bend LFO Pitch", 0, 127, 0);
    addInt(ids::bendLfoFilter, "Pitch Bend LFO Filter", 0, 127, 0);
    addInt(ids::bendLfoAmplitude, "Pitch Bend LFO Amp", 0, 127, 0);
    addInt(ids::aftertouchPitch, "Aftertouch Pitch", -24, 24, 0);
    addInt(ids::aftertouchFilter, "Aftertouch Filter", -64, 63, 0);
    addInt(ids::aftertouchAmplitude, "Aftertouch Amplitude", -64, 63, 0);
    addInt(ids::aftertouchLfoPitch, "Aftertouch LFO Pitch", 0, 127, 0);
    addInt(ids::aftertouchLfoFilter, "Aftertouch LFO Filter", 0, 127, 0);
    addInt(ids::aftertouchLfoAmplitude, "Aftertouch LFO Amp", 0, 127, 0);
    layout.add(std::make_unique<juce::AudioParameterBool>(ids::portamentoSwitch,
                                                          "Portamento", false));
    addInt(ids::portamentoTime, "Portamento Time", 0, 127, 0);
    addInt(ids::pitchEgInitial, "Pitch EG Initial Level", -64, 63, 0);
    addInt(ids::pitchEgAttack, "Pitch EG Attack Time", -64, 63, 0);
    addInt(ids::pitchEgReleaseLevel, "Pitch EG Release Level", -64, 63, 0);
    addInt(ids::pitchEgReleaseTime, "Pitch EG Release Time", -64, 63, 0);
    for (const auto& binding : effectBindings())
    {
        if (binding.encoding == EffectEncoding::type)
        {
            juce::StringArray choices;
            for (const auto& type : qy70::effectTypes(binding.block))
                choices.add(juce::String::fromUTF8(type.name.data(),
                                                   static_cast<int>(type.name.size())));
            addChoice(binding.id.toRawUTF8(), binding.name.toRawUTF8(),
                      choices, binding.defaultValue);
        }
        else if (binding.id == "variationConnection")
            addChoice(binding.id.toRawUTF8(), binding.name.toRawUTF8(),
                      { "Insertion", "System" }, binding.defaultValue);
        else if (binding.id == "variationPart")
        {
            juce::StringArray choices;
            for (int part = 1; part <= 32; ++part) choices.add("Part " + juce::String(part));
            choices.add("Off");
            addChoice(binding.id.toRawUTF8(), binding.name.toRawUTF8(),
                      choices, binding.defaultValue);
        }
        else
            addInt(binding.id.toRawUTF8(), binding.name.toRawUTF8(),
                   binding.minimum, binding.maximum, binding.defaultValue);
    }
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QY70ControllerAudioProcessor();
}
