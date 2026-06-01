#pragma once

#include <JuceHeader.h>

namespace audio
{

enum class SoundID
{
    tilePlace,
    claim,
    complete,
    rotate,
    gameOver,
    count
};

class SoundBank
{
public:
    SoundBank();
    ~SoundBank();

    void play (SoundID id, float gain = 1.0f);

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::MixerAudioSource   mixer;
    juce::AudioSourcePlayer   player;

    struct Sound
    {
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        std::unique_ptr<juce::MemoryBlock>             data;
    };

    std::array<juce::AudioBuffer<float>, (size_t) SoundID::count> buffers;

    struct Voice
    {
        juce::AudioBuffer<float>* buffer = nullptr;
        int                       position = 0;
        float                     gain = 1.0f;
        bool                      active = false;
    };

    static constexpr int kMaxVoices = 16;

    class VoiceMixer : public juce::AudioSource
    {
    public:
        void prepareToPlay (int, double) override {}
        void releaseResources() override {}
        void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override;

        std::array<Voice, kMaxVoices> voices {};
        juce::SpinLock lock;
    };

    VoiceMixer voiceMixer;

    void loadSound (SoundID id, const void* data, int size);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundBank)
};

} // namespace audio
