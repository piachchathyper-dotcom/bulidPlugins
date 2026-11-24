#include "PluginEditor.h"

MultiRecEditor::MultiRecEditor (MultiRecProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    addAndMakeVisible (recButton);
    addAndMakeVisible (folderButton);
    addAndMakeVisible (info);
    info.setJustificationType (juce::Justification::centred);

    recButton.onClick = [this]
    {
        if (proc.isRecording()) proc.stopRecording();
        else                    proc.startRecording();
        refresh();
    };

    folderButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Pick output folder", proc.getOutputFolder());
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto r = fc.getResult();
                                  if (r.isDirectory()) proc.setOutputFolder (r);
                                  refresh();
                              });
    };

    setSize (380, 150);
    startTimerHz (5);
    refresh();
}

void MultiRecEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MultiRecEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto top = r.removeFromTop (44);
    recButton.setBounds (top.removeFromLeft (r.getWidth() / 2 - 4));
    folderButton.setBounds (top.withTrimmedLeft (8));
    info.setBounds (r.withTrimmedTop (8));
}

void MultiRecEditor::refresh()
{
    const bool rec = proc.isRecording();
    recButton.setButtonText (rec ? "STOP" : "REC");
    recButton.setColour (juce::TextButton::buttonColourId, rec ? juce::Colours::red : juce::Colours::darkgrey);

    juce::String status;
    if (rec)
    {
        const int s = (int) proc.getRecordedSeconds();
        status = "Recording " + juce::String::formatted ("%02d:%02d", s / 60, s % 60)
                 + "  (" + juce::String (proc.getActiveInputs()) + " input)";
    }
    else
        status = "Idle";

    info.setText (status + "\n" + proc.getOutputFolder().getFullPathName(), juce::dontSendNotification);
}
