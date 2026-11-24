#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Records every enabled input bus to its own WAV (24-bit). Audio passes through untouched.
// numInputBuses = how many input buses (each can be mono/stereo/multichannel). Change if you want more.
class MultiRecProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numInputBuses = 2;

    MultiRecProcessor();
    ~MultiRecProcessor() override;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MultiRec"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording.load(); }
    int  getActiveInputs() const { return activeInputs.load(); }
    double getRecordedSeconds() const { return getSampleRate() > 0 ? (double) recordedSamples.load() / getSampleRate() : 0.0; }

    juce::File getOutputFolder() const { return outputFolder; }
    void setOutputFolder (const juce::File& f) { outputFolder = f; }

private:
    static BusesProperties makeBuses();

    juce::File outputFolder { juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("MultiRec") };

    juce::TimeSliceThread backgroundThread { "MultiRec writer" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writers[numInputBuses];
    int writerChannels[numInputBuses] {};
    juce::CriticalSection writerLock;

    std::atomic<bool> recording { false };
    std::atomic<int> activeInputs { 0 };
    std::atomic<juce::int64> recordedSamples { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiRecProcessor)
};
