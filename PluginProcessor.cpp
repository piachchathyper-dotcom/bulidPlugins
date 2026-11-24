#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor::BusesProperties MultiRecProcessor::makeBuses()
{
    BusesProperties p;
    for (int i = 0; i < MultiRecProcessor::numInputBuses; ++i)
        p = p.withInput ("Input " + juce::String (i + 1), juce::AudioChannelSet::stereo(), true);
    return p.withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}

MultiRecProcessor::MultiRecProcessor() : AudioProcessor (makeBuses()) {}

MultiRecProcessor::~MultiRecProcessor()
{
    stopRecording();
    backgroundThread.stopThread (2000);
}

bool MultiRecProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto ok = [] (const juce::AudioChannelSet& s) { return s.isDisabled() || s.size() <= 8; };

    if (l.getMainInputChannelSet().isDisabled()) return false;
    if (l.getMainOutputChannelSet() != l.getMainInputChannelSet()) return false;
    for (auto& s : l.inputBuses)
        if (! ok (s)) return false;
    return true;
}

void MultiRecProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const juce::ScopedTryLock sl (writerLock);
    if (! sl.isLocked() || ! recording.load())
        return;

    const int n = buffer.getNumSamples();
    for (int b = 0; b < numInputBuses; ++b)
    {
        if (writers[b] == nullptr) continue;
        auto in = getBusBuffer (buffer, true, b);
        if (in.getNumChannels() != writerChannels[b]) continue; // host changed layout mid-take
        writers[b]->write (in.getArrayOfReadPointers(), n);
    }
    recordedSamples += n;
}

void MultiRecProcessor::startRecording()
{
    if (recording.load()) return;

    outputFolder.createDirectory();
    if (! backgroundThread.isThreadRunning())
        backgroundThread.startThread();

    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
    juce::WavAudioFormat wav;
    int active = 0;

    const juce::ScopedLock sl (writerLock);
    for (int b = 0; b < numInputBuses; ++b)
    {
        auto* bus = getBus (true, b);
        if (bus == nullptr || ! bus->isEnabled()) continue;
        const int ch = bus->getNumberOfChannels();
        if (ch <= 0) continue;

        auto file = outputFolder.getNonexistentChildFile (stamp + "_In" + juce::String (b + 1), ".wav");
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
        if (stream == nullptr) continue;

        std::unique_ptr<juce::AudioFormatWriter> w (
            wav.createWriterFor (stream.get(), getSampleRate(), (unsigned int) ch, 24, {}, 0));
        if (w == nullptr) continue;

        stream.release(); // writer owns it now
        writers[b].reset (new juce::AudioFormatWriter::ThreadedWriter (w.release(), backgroundThread, 65536));
        writerChannels[b] = ch;
        ++active;
    }

    recordedSamples = 0;
    activeInputs = active;
    recording = active > 0;
}

void MultiRecProcessor::stopRecording()
{
    const juce::ScopedLock sl (writerLock);
    recording = false;
    activeInputs = 0;
    for (auto& w : writers)
        w.reset(); // flushes + closes file
}

juce::AudioProcessorEditor* MultiRecProcessor::createEditor() { return new MultiRecEditor (*this); }

void MultiRecProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream out (dest, false);
    out.writeString (outputFolder.getFullPathName());
}

void MultiRecProcessor::setStateInformation (const void* data, int size)
{
    juce::MemoryInputStream in (data, (size_t) size, false);
    auto path = in.readString();
    if (juce::File::isAbsolutePath (path))
        outputFolder = juce::File (path);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MultiRecProcessor(); }
