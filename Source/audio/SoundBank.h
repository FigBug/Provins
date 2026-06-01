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

    void play (SoundID id, float gain = 1.0f, float pan = 0.0f);

    void  setVolume (float v) noexcept { masterVolume = juce::jlimit (0.0f, 1.0f, v); }
    float getVolume() const noexcept   { return masterVolume; }

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::AudioSourcePlayer   player;

    std::array<juce::AudioBuffer<float>, (size_t) SoundID::count> buffers;
    float masterVolume = 1.0f;

    struct Voice
    {
        juce::AudioBuffer<float>* buffer = nullptr;
        int                       position = 0;
        float                     gain = 1.0f;
        float                     pan  = 0.0f;
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
        std::atomic<float> volume { 1.0f };
    };

    VoiceMixer voiceMixer;

    void loadSound (SoundID id, const void* data, int size);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundBank)
};

} // namespace audio
