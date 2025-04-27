/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_analytics/juce_analytics.h>
#include <juce_animation/juce_animation.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "BinaryData.h"
#include "PluginProcessor.h"
#include <future>

//==============================================================================
/**
*/

class SenderAudioProcessorEditor  : public juce::AudioProcessorEditor, 
                                    public juce::Slider::Listener, 
                                    public juce::Button::Listener, 
                                    public juce::Timer 

{
    public:
    SenderAudioProcessorEditor (SenderAudioProcessor&);
    ~SenderAudioProcessorEditor() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged (juce::Slider *slider) override;
    void buttonClicked (juce::Button *button) override;
    int32_t handleAuth (juce::String username, juce::String password, SenderAudioProcessor& audioProcessor);
    void handleSuccessfulAuth();
    int32_t handleDataChannel (juce::String workspace, juce::String stream_type);
    void timerCallback() override;

    void disconnectTab();
    void disconnectionError();

    private:
    SenderAudioProcessor& audioProcessor;
    
    juce::Image corelink_logo;

    juce::Label workspace_label;
    juce::Label stream_type_label;
    juce::Label host_id_label;
    juce::Label buffer_size_label;
    juce::Label dir_path_label;
    juce::Label username_label;
    juce::Label password_label;
    
    juce::TextEditor workspace_edit;
    juce::TextEditor stream_type_edit;
    juce::TextEditor host_id_edit;
    juce::TextEditor buffer_size_edit;
    juce::TextEditor dir_path_edit;
    juce::TextEditor username_editor;
    juce::TextEditor password_editor;
    
    juce::TextButton submitBtn;
    juce::TextButton signInBtn;
    juce::TextButton disconnectBtn;
    
    juce::Slider vol_slider;
    
    juce::TextButton tabSignIn;
    juce::TextButton tabAddConnection;
    juce::TextButton tabControl;
    juce::TextButton tabSavedConnection;
    juce::ToggleButton showPasswordToggle;
    juce::AudioFormatManager formatManager;
    juce::Image logo;

    std::future<int32_t> userValidationStatusCode;
    std::future<int32_t> authResStatusCode;
    std::future<int32_t> dataChannelStatusCode;

    double currentPercentage = 0.0;
    double currentValue = 0.0;
    int runLength = 500;
    bool isProgressBarVisible = false;
    std::unique_ptr<juce::ProgressBar> progressBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SenderAudioProcessorEditor)
};

