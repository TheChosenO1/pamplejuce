/**
 * @file
 * @brief This file contains the basic framework code for a JUCE audio sender plugin
 * @date 2024-03-08
*/

#pragma once

#define CORELINK_USE_TCP
#define CORELINK_USE_CONCURRENT_COUNTER
#define CORELINK_USE_CONCURRENT_QUEUE
#define CORELINK_ENABLE_STRING_UTIL_FUNCTIONS
#define NUMBER_CHANNEL 4
#define JITTER_ESTIMATION_STREAM_TYPE "JitterEst"

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
#include "corelink_all.hpp"
#include <iostream>
#include <future>
#include "JitterBuffer.h"
#include <cmath>

#include "CorelinkAudio.h"
#include "ThreadSafeVar.h"

template<typename t> using in = corelink::in<t>;
template<typename t> using out = corelink::out<t>;

class CorelinkClient;

class SenderAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    SenderAudioProcessor();
    ~SenderAudioProcessor() override;

    void prepareToPlay (double mSampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    #ifndef JucePlugin_PreferredChannelConfigurations
      bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    #endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* mData, int sizeInBytes) override;

    void onChannelInit(corelink::core::network::channel_id_type host_id);
    void onChannelUninit(corelink::core::network::channel_id_type host_id);
    void onError(corelink::core::network::channel_id_type host_id, in<std::string> err);
    void addOnSubscribeHandler(corelink::core::network::channel_id_type mControlChannelId,
                                                    out<corelink::client::corelink_classic_client> mClient);

    template<class T>
    void swapMove(T& a, T& b);

    void sendData(juce::AudioBuffer<float> buffer, int mAudioBufferSize, std::vector<uint8_t> &mData, int numChannels);

    void setAudioWorkspace(const juce::String& val);
    void setAudioStreamType(const juce::String& val);
    void setStreamInit();
    void setHostId(const juce::String& val);
    void setBufferSize(const juce::String& val);
    void setVolume(float newVolume);

    bool getStreamInit();
    juce::String getAudioWorkspace();
    juce::String getAudioStreamType();
    juce::String getHostId();
    int getBufferSize();
    float getVolume();

    void createSender(const juce::String& workspace, const juce::String& stream_type);
    void createReceiver();
    void setupControlChannel(const juce::String& hostId, const juce::String& username, const juce::String& password);

    int32_t getAuthStatusCode();
    int32_t getCreateSenderStatusCode();
    bool getHandledAuth();
    void waitForHandledAuth(bool);
    void waitForMLoading(bool);
    bool getMLoading() const;
    void resetHandledAuth();
    void disconnectControlChannel();
    int getNMeasurement();
    
    //Testing Methods
    bool getMDone() const;
    bool doesCorelinkClientExist() const;
    double getAudioSampleRate() const;
    int getAudioBufferSize() const;
    

    using ProgressCallback = std::function<void(double)>;
    void setProgressCallback(ProgressCallback callback);

//==============================================================================
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SenderAudioProcessor)
    int32_t authStatusCode = 999;
    int32_t createSenderStatusCode = 999;

    int mAudioBufferSize;
    double mAudioSampleRate;

    ThreadSafeVar<bool> mDone;
    ThreadSafeVar<bool> mError; 
    ThreadSafeVar<bool> mLoading;
    ThreadSafeVar<bool> mStreamInit;
    ThreadSafeVar<bool> handledAuth;
    ThreadSafeVar<float> mVolume;
    std::atomic<int> nMeasurement = 1;
    std::atomic<int> mSenderStreamID; 
    std::atomic<int> mReceiverStreamID = 0;


    std::string mAudioWorkspace   = "ZackAudio";
    std::string mAudioStreamType  = "audiotesting";
    juce::String mCorelinkHostId   = "127.0.0.1";
    int          mJitterBufferSize = 25;

    int         mPacketCounter = 0;

    std::vector<uint8_t> mData;

    std::unique_ptr<JitterBuffer> mJitterBuffer;
    std::string mUsername;
    juce::ThreadPool mThreadPool{4};

    std::atomic<float*> mjitterBuffer;
    juce::MemoryBlock mBlock;

    ProgressCallback progressCallback;
    std::unique_ptr<CorelinkClient> mCorelinkClient;

};
