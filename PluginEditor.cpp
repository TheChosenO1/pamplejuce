/**
 * @file
 * @brief This file contains the basic framework code for a JUCE audio sender editor
 * @date 2024-03-08
*/

#include "PluginEditor.h"

SenderAudioProcessorEditor::SenderAudioProcessorEditor (SenderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    logo = juce::ImageCache::getFromMemory(BinaryData::Corelink_Logo_png, BinaryData::Corelink_Logo_pngSize);
    addAndMakeVisible(vol_slider);

    vol_slider.setValue(0.5f);
    vol_slider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    vol_slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, true, 30, 20);

    vol_slider.setRange(0, 1);
    vol_slider.setNumDecimalPlacesToDisplay(2);

    vol_slider.setSliderStyle(juce::Slider::LinearHorizontal);
    vol_slider.setColour(juce::Slider::backgroundColourId, juce::Colours::black);

    addAndMakeVisible(tabSignIn);
    addAndMakeVisible(tabAddConnection);
    addAndMakeVisible(tabControl);
    addAndMakeVisible(tabSavedConnection);

    tabSignIn.addListener(this);
    tabAddConnection.addListener(this);
    tabControl.addListener(this);
    tabSavedConnection.addListener(this);
    signInBtn.addListener(this);

    tabSignIn.setEnabled(true);
    tabSignIn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    tabSignIn.setButtonText("SignIn");
    tabSignIn.setConnectedEdges(juce::TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

    tabAddConnection.setEnabled(false);
    tabAddConnection.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    tabAddConnection.setButtonText("AddConnection");
    tabAddConnection.setConnectedEdges(juce::TextButton::ConnectedEdgeFlags::ConnectedOnRight);

    tabControl.setEnabled(false);
    tabControl.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    tabControl.setButtonText("Control");
    tabControl.setConnectedEdges(juce::TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

    tabSavedConnection.setEnabled(false);
    tabSavedConnection.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    tabSavedConnection.setButtonText("SaveConnection");
    tabSavedConnection.setConnectedEdges(juce::TextButton::ConnectedEdgeFlags::ConnectedOnTop);
    tabSavedConnection.setConnectedEdges(juce::TextButton::ConnectedEdgeFlags::ConnectedOnRight);

    addAndMakeVisible(workspace_label);
    workspace_label.setText("Workspace", juce::dontSendNotification);
    workspace_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(workspace_edit);

    addAndMakeVisible(stream_type_label);
    stream_type_label.setText("Stream Type", juce::dontSendNotification);
    stream_type_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(stream_type_edit);

    addAndMakeVisible(host_id_label);
    host_id_label.setText("Host ID", juce::dontSendNotification);
    host_id_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(host_id_edit);

    addAndMakeVisible(buffer_size_label);
    buffer_size_label.setText("Buffering Size", juce::dontSendNotification);
    buffer_size_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(buffer_size_edit);

    addAndMakeVisible(username_label);
    username_label.setText("Username", juce::dontSendNotification);
    username_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(username_editor);

    addAndMakeVisible(password_label);
    password_label.setText("Password", juce::dontSendNotification);
    password_label.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(password_editor);
    password_editor.setPasswordCharacter('*');
    
    addAndMakeVisible(showPasswordToggle);
    showPasswordToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::black);
    showPasswordToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::black);
    showPasswordToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::darkgrey);
    showPasswordToggle.setButtonText("Show");
    showPasswordToggle.setToggleState(false, juce::dontSendNotification);
    showPasswordToggle.addListener(this);
    
    submitBtn.setButtonText("Connect Sender");

    addAndMakeVisible(signInBtn);
    signInBtn.setEnabled(true);
    signInBtn.setButtonText("Sign In");
    
    addAndMakeVisible(disconnectBtn);
    disconnectBtn.setButtonText("Disconnect Sender");
    disconnectBtn.setVisible(false);

    host_id_edit.setText("127.0.0.1");
    workspace_edit.setText("Holodeck");
    stream_type_edit.setText("test");

    submitBtn.onClick = [this] () -> void {
        audioProcessor.setAudioWorkspace(workspace_edit.getText());
        audioProcessor.setAudioStreamType(stream_type_edit.getText());
        dataChannelStatusCode = std::async(std::launch::async,
                                           &SenderAudioProcessorEditor::handleDataChannel,
                                           this,
                                           workspace_edit.getText(),
                                           stream_type_edit.getText());
        
        const int32_t res = dataChannelStatusCode.get();
        this->repaint();
    };

    signInBtn.onClick = [this] () -> void {
        audioProcessor.setHostId(host_id_edit.getText());
        authResStatusCode = std::async(std::launch::async,
                                       &SenderAudioProcessorEditor::handleAuth,
                                       this,
                                       username_editor.getText(),
                                       password_editor.getText(),
                                       std::ref(audioProcessor));
        const int32_t res = authResStatusCode.get();
        if (res == 0) {
            handleSuccessfulAuth();
        } else {
            audioProcessor.disconnectControlChannel();
        }

    };
    
    disconnectBtn.onClick = [this] {
        audioProcessor.disconnectControlChannel();
        disconnectionError();
    };

    setSize (400, 300);
    setResizable(true, true);
    vol_slider.addListener(this);
    vol_slider.setVelocityBasedMode(true);
    vol_slider.setVelocityModeParameters(1.0);

    vol_slider.setVisible(false);
    workspace_label.setVisible(false);
    workspace_edit.setVisible(false);
    stream_type_label.setVisible(false);
    stream_type_edit.setVisible(false);
    buffer_size_label.setVisible(false);
    buffer_size_edit.setVisible(false);
    submitBtn.setVisible(false);
    disconnectBtn.setVisible(false);

}

SenderAudioProcessorEditor::~SenderAudioProcessorEditor()
{
    //meterThread.stopThread(1000);
}

void SenderAudioProcessorEditor::handleSuccessfulAuth()
{
    tabSignIn.setEnabled(false);
    tabAddConnection.setEnabled(false);
    
    username_label.setVisible(false);
    username_editor.setVisible(false);
    password_label.setVisible(false);
    password_editor.setVisible(false);
    signInBtn.setEnabled(false);
    signInBtn.setVisible(false);
    host_id_edit.setVisible(false);

    tabControl.setEnabled(true);
    tabSavedConnection.setEnabled(true);

    workspace_label.setVisible(true);
    workspace_edit.setVisible(true);
    stream_type_label.setVisible(true);
    stream_type_edit.setVisible(true);
    buffer_size_label.setVisible(true);
    buffer_size_edit.setVisible(true);
    submitBtn.setVisible(true);
    disconnectBtn.setVisible(false);
}

void SenderAudioProcessorEditor::disconnectionError(){
    juce::MessageManager::callAsync([]() {
           juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                  "Disconnect",
                                                  "Sender is disconnected");
       });
    
    tabSignIn.setEnabled(false);
    tabAddConnection.setEnabled(false);
    username_label.setVisible(false);
    username_editor.setVisible(false);
    password_label.setVisible(false);
    password_editor.setVisible(false);
    signInBtn.setEnabled(false);
    signInBtn.setVisible(false);
    host_id_edit.setVisible(false);
    showPasswordToggle.setVisible(false);
    

    tabControl.setEnabled(true);
    tabSavedConnection.setEnabled(true);

    workspace_label.setVisible(true);
    workspace_edit.setVisible(true);
    stream_type_label.setVisible(true);
    stream_type_edit.setVisible(true);
    buffer_size_label.setVisible(true);
    buffer_size_edit.setVisible(true);

    submitBtn.setVisible(true);
    disconnectBtn.setVisible(false);
}

int32_t SenderAudioProcessorEditor::handleDataChannel(juce::String workspace, juce::String stream_type) {
    audioProcessor.createSender(workspace, stream_type);
    juce::MessageManager::callAsync([this]() {
        progressBar = std::make_unique<juce::ProgressBar>(currentPercentage);
        addAndMakeVisible(progressBar.get());
        isProgressBarVisible = true;
        resized();
    });
    juce::MessageManager::callAsync([this]() {
        currentPercentage = 0.0;
        currentValue = 0.0;
        progressBar = std::make_unique<juce::ProgressBar>(currentPercentage);
        addAndMakeVisible(progressBar.get());
        isProgressBarVisible = true;
        resized();
    });

    startTimer(5);
    audioProcessor.waitForMLoading(false);

    return 0;
}

void SenderAudioProcessorEditor::timerCallback() {
    if (currentValue < runLength) {
        currentValue += 1;
        currentPercentage = static_cast<double>(currentValue) / runLength;
        repaint();
    } else {
        stopTimer();
        currentValue = 0;

        // Hold the current progress value for 2 seconds before removing the progress bar
         juce::Timer::callAfterDelay(2000, [this]() {
             if (progressBar != nullptr) {
                 removeChildComponent(progressBar.get());
                 progressBar.reset();
             }
             isProgressBarVisible = false;
             disconnectBtn.setVisible(true);
             resized();
         });
    }
}

int32_t SenderAudioProcessorEditor::handleAuth(juce::String username, juce::String password, SenderAudioProcessor& audioProcessor) {
    audioProcessor.setupControlChannel(host_id_edit.getText(), username_editor.getText(), password_editor.getText());
    audioProcessor.waitForHandledAuth(true);
    int32_t res_code = audioProcessor.getAuthStatusCode();
    if (res_code != 0) {
      // Authentication Failed
      audioProcessor.resetHandledAuth();
      juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Authentication Failed",
                "Invalid username or password. Please try again.",
                "OK"
            );
    } else {}
    return res_code;
}

void SenderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);

    g.setColour (juce::Colours::black);
    g.setFont (15.0f);

    juce::Rectangle<int> bounds = getLocalBounds();

    if (audioProcessor.getNMeasurement() == 10000 && !tabAddConnection.isEnabled()) {
        addAndMakeVisible(submitBtn);
    }

    g.setOpacity(0.1f);
    int logoWidth = bounds.getWidth() / 1.5;
    int logoHeight = bounds.getHeight() / 1.5;
    int centerX = bounds.getX() + (bounds.getWidth() - logoWidth) / 2;
    int centerY = bounds.getY() + (bounds.getHeight() - logoHeight) / 2;
    g.drawImageWithin(logo, centerX, centerY, logoWidth, logoHeight, juce::RectanglePlacement::centred);
    g.setOpacity(1.0f);

    this->host_id_label.attachToComponent(&host_id_edit, true);
    host_id_edit.setBounds(bounds.getWidth()/2 - 30, bounds.getHeight()/2 - 80, 130, 22);

    this->username_label.attachToComponent(&username_editor, true);
    username_editor.setBounds(bounds.getWidth()/2 - 30, bounds.getHeight()/2 - 40, 130, 22);

    this->password_label.attachToComponent(&password_editor, true);
    password_editor.setBounds(bounds.getWidth()/2 - 30, bounds.getHeight()/2, 130, 22);

    signInBtn.setBounds(bounds.getWidth()/2 - 75/2, bounds.getHeight()/2 + 45, 80, 27);

    this->workspace_label.attachToComponent(&workspace_edit, true);
    workspace_edit.setBounds(bounds.getWidth()/2, bounds.getHeight()/2 - 40, 100, 20);

    this->stream_type_label.attachToComponent(&stream_type_edit, true);
    stream_type_edit.setBounds(bounds.getWidth()/2, bounds.getHeight()/2, 100, 20);

    submitBtn.setBounds(bounds.getWidth()/2 - 150/2, bounds.getHeight()/2 + 40, 150, 30);

    if (!tabAddConnection.isEnabled() && audioProcessor.getHandledAuth()) {
        int offset = 50;
        if (!audioProcessor.getMLoading()) {
            repaint();
            offset = 100;
        }
        g.drawText("Host ID:", bounds.getWidth()/2 - offset, bounds.getHeight()/2 - 80, 100, 20, juce::Justification::left, true);
        g.drawText(audioProcessor.getHostId(), bounds.getWidth()/2 + 10, bounds.getHeight()/2 - 80, 150, 20, juce::Justification::left, true);
    } else {
        repaint();
    }

    if (audioProcessor.getHandledAuth() && !audioProcessor.getMLoading() && !tabAddConnection.isEnabled()) {
        g.drawText("Workspace:", bounds.getWidth()/2 - 100, bounds.getHeight()/2 - 40, 100, 20, juce::Justification::left, true);
        g.drawText(audioProcessor.getAudioWorkspace(), bounds.getWidth()/2 + 10, bounds.getHeight()/2 - 40, 150, 20, juce::Justification::left, true);

        g.drawText("Stream Type:", bounds.getWidth()/2 - 100, bounds.getHeight()/2, 100, 20, juce::Justification::left, true);
        g.drawText(audioProcessor.getAudioStreamType(), bounds.getWidth()/2 + 10, bounds.getHeight()/2, 150, 20, juce::Justification::left, true);

        workspace_edit.clear();
        workspace_edit.setVisible(false);

        stream_type_edit.clear();
        stream_type_edit.setVisible(false);

        buffer_size_edit.clear();
        buffer_size_edit.setVisible(false);

        submitBtn.setVisible(false);
    } else {
      repaint();
    }

    vol_slider.setColour(juce::Slider::ColourIds::backgroundColourId, juce::Colours::black);
    vol_slider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colour(0xBF000000));
    vol_slider.setColour(juce::Slider::ColourIds::textBoxTextColourId, juce::Colours::white);
    vol_slider.setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::blue);
}

void SenderAudioProcessorEditor::resized()
{
    vol_slider.setBounds(getRight()/4, getHeight()/2 - 60 - 60, getWidth()/2, 40);

    tabSignIn.setBounds(0, 0, getRight()/4, 15);
    tabAddConnection.setBounds(getRight()/4, 0, getRight()/4, 15);
    tabControl.setBounds(getRight()/2, 0, getRight()/4, 15);
    tabSavedConnection.setBounds(getRight()*3/4, 0, getRight()/4, 15);
    showPasswordToggle.setBounds(getRight()*4/5 - 15, getHeight()*2/3 - 50, 150, 25);
    if (isProgressBarVisible) {
        progressBar ->setBounds(20, 40, getWidth() - 40, 20);
    }
    disconnectBtn.setBounds(getWidth()/2 - 75/2, getHeight()/2 + 80, 75, 30);
}

void SenderAudioProcessorEditor::sliderValueChanged (juce::Slider *slider)
{
    audioProcessor.setVolume((float) slider->getValue());
}

void SenderAudioProcessorEditor::buttonClicked (juce::Button *button)
{
    if (button == &tabAddConnection)
    {
        tabSignIn.setEnabled(false);
        tabAddConnection.setEnabled(false);
        tabControl.setEnabled(true);
        tabSavedConnection.setEnabled(true);

        vol_slider.setVisible(false);
        workspace_label.setVisible(true);
        workspace_edit.setVisible(true);
        stream_type_label.setVisible(true);
        stream_type_edit.setVisible(true);
        host_id_label.setVisible(true);
        buffer_size_label.setVisible(true);
        buffer_size_edit.setVisible(true);
        submitBtn.setVisible(true);
        disconnectBtn.setVisible(false);
        showPasswordToggle.setVisible(false);
    }
    else if (button == &tabControl)
    {
        tabSignIn.setEnabled(false);
        tabAddConnection.setEnabled(true);
        tabControl.setEnabled(false);
        tabSavedConnection.setEnabled(true);

        vol_slider.setVisible(true);
        workspace_label.setVisible(false);
        workspace_edit.setVisible(false);
        stream_type_label.setVisible(false);
        stream_type_edit.setVisible(false);
        host_id_label.setVisible(false);
        host_id_edit.setVisible(false);
        buffer_size_label.setVisible(false);
        buffer_size_edit.setVisible(false);
        submitBtn.setVisible(false);
        disconnectBtn.setVisible(false);
        showPasswordToggle.setVisible(false);
    }
    else if (button == &tabSavedConnection)
    {
        tabSignIn.setEnabled(false);
        tabAddConnection.setEnabled(true);
        tabControl.setEnabled(true);
        tabSavedConnection.setEnabled(false);

        vol_slider.setVisible(false);
        workspace_label.setVisible(false);
        workspace_edit.setVisible(false);
        stream_type_label.setVisible(false);
        stream_type_edit.setVisible(false);
        host_id_label.setVisible(false);
        host_id_edit.setVisible(false);
        buffer_size_label.setVisible(false);
        buffer_size_edit.setVisible(false);
        submitBtn.setVisible(false);
        disconnectBtn.setVisible(false);
        showPasswordToggle.setVisible(false);
    }
    else if (button == &showPasswordToggle)
    {
        password_editor.setPasswordCharacter(showPasswordToggle.getToggleState() ? 0 : '*');
    }
}
