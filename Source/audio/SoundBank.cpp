#include "SoundBank.h"
#include "BinaryData.h"

namespace audio
{

SoundBank::SoundBank()
{
    formatManager.registerBasicFormats();

    loadSound (SoundID::tilePlace, BinaryData::tile_place_ogg, BinaryData::tile_place_oggSize);
    loadSound (SoundID::claim,     BinaryData::claim_ogg,      BinaryData::claim_oggSize);
    loadSound (SoundID::complete,  BinaryData::complete_ogg,   BinaryData::complete_oggSize);
    loadSound (SoundID::rotate,    BinaryData::rotate_ogg,     BinaryData::rotate_oggSize);
    loadSound (SoundID::gameOver,  BinaryData::gameover_ogg,   BinaryData::gameover_oggSize);

    auto err = deviceManager.initialiseWithDefaultDevices (0, 2);
    if (err.isNotEmpty())
        DBG ("Audio init error: " + err);

    player.setSource (&voiceMixer);
    deviceManager.addAudioCallback (&player);
}

SoundBank::~SoundBank()
{
    deviceManager.removeAudioCallback (&player);
    player.setSource (nullptr);
}

void SoundBank::loadSound (SoundID id, const void* data, int size)
{
    auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (std::move (stream)));

    if (reader == nullptr)
        return;

    auto& buf = buffers[(size_t) id];
    buf.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read (&buf, 0, (int) reader->lengthInSamples, 0, true, true);
}

void SoundBank::play (SoundID id, float gain)
{
    auto& buf = buffers[(size_t) id];
    if (buf.getNumSamples() == 0)
        return;

    juce::SpinLock::ScopedLockType sl (voiceMixer.lock);
    for (auto& v : voiceMixer.voices)
    {
        if (! v.active)
        {
            v.buffer   = &buf;
            v.position = 0;
            v.gain     = gain;
            v.active   = true;
            return;
        }
    }
}

void SoundBank::VoiceMixer::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    juce::SpinLock::ScopedLockType sl (lock);
    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        int samplesRemaining = v.buffer->getNumSamples() - v.position;
        int samplesToWrite   = juce::jmin (info.numSamples, samplesRemaining);

        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            int srcCh = juce::jmin (ch, v.buffer->getNumChannels() - 1);
            info.buffer->addFrom (ch, info.startSample,
                                  *v.buffer, srcCh, v.position,
                                  samplesToWrite, v.gain);
        }

        v.position += samplesToWrite;
        if (v.position >= v.buffer->getNumSamples())
            v.active = false;
    }
}

} // namespace audio
