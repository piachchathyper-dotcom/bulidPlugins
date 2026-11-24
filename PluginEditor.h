#pragma once
#include "PluginProcessor.h"

class MultiRecEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MultiRecEditor (MultiRecProcessor&);
    ~MultiRecEditor() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { refresh(); }
    void refresh();

    MultiRecProcessor& proc;
    juce::TextButton recButton { "REC" }, folderButton { "Folder..." };
    juce::Label info;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiRecEditor)
};
