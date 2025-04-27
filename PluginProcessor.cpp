/**
 * @file
 * @brief This file contains the basic framework code for a JUCE audio sender plugin
 * @date 2024-03-08
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CorelinkClient.h"

/**
 * @brief Constructor for the SenderAudioProcessor class
*/
SenderAudioProcessor::SenderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                     .withInput("Input", juce::AudioChannelSet::discreteChannels(NUMBER_CHANNEL), true)
#endif
                     .withOutput("Output", juce::AudioChannelSet::discreteChannels(NUMBER_CHANNEL), true)
#endif
                         )
#endif
{
    mDone.set(false);
    mLoading.set(true);
    mStreamInit.set(false);
    mCorelinkClient = std::make_unique<CorelinkClient>();

}

/**
 * @brief
*/
void SenderAudioProcessor::setupControlChannel(const juce::String& hostId, const juce::String& username, const juce::String& password)
{

    mCorelinkClient->setInfo(hostId, username);


    if (!mCorelinkClient->initProtocols())
    {
        throw std::runtime_error("Failed to initialize protocol information. Please contact corelink development");
        DBG("Failed to initiate protocol");
    }

    mCorelinkClient->addControlChannel(this);

    // Wait for connection to establish
    mDone.waitForValue(true);
    mDone.set(false);

    mCorelinkClient->authenticate(username, password, [&, username](int statusCode) {
        if (statusCode == 0) {
            authStatusCode = statusCode;
            std::string workspace = "Holodeck";
            std::string streamType = JITTER_ESTIMATION_STREAM_TYPE;
            std::string name = username.toStdString();

            addOnSubscribeHandler(mCorelinkClient->mControlChannelId, mCorelinkClient->mClient);
            mJitterBuffer    = std::make_unique<JitterBuffer>(mCorelinkClient->mClient, mCorelinkClient->mControlChannelId, workspace, streamType, name);
            mJitterBuffer->setupSender();
        } else {
            DBG("Failed to authenticate sender");
        }
        handledAuth.set(true);
    });
}

void SenderAudioProcessor::disconnectControlChannel() {
    std::vector<corelink::core::network::channel_id_type> hostIds = {mCorelinkClient->mStreamId, mJitterBuffer->getStreamId()};
    mCorelinkClient->disconnectChannel(hostIds, [this](int statusCode, corelink::core::network::channel_id_type channelId) {
            if (statusCode == 0) {
                std::cout << "Sender channel session with ID " << channelId << " was purged\n";
                mSenderStreamID = -1;
                mLoading.set(true);
            } else {
                std::cerr << "Failed to disconnect stream. Status: " <<statusCode << "\n";
            }
        }
    );
}

void SenderAudioProcessor::setProgressCallback(ProgressCallback callback) {
    progressCallback = callback;
}

int32_t SenderAudioProcessor::getAuthStatusCode() {
    return authStatusCode;
}

int32_t SenderAudioProcessor::getCreateSenderStatusCode() {
    return createSenderStatusCode;
}

void SenderAudioProcessor::waitForHandledAuth(bool new_value) {
    handledAuth.waitForValue(new_value);
}

bool SenderAudioProcessor::getHandledAuth() {
    return handledAuth.get();
}

void SenderAudioProcessor::waitForMLoading(bool new_value)
{
    mLoading.waitForValue(new_value);
}

bool SenderAudioProcessor::getMLoading() const
{
    return mLoading.get();
}

void SenderAudioProcessor::resetHandledAuth()
{
    handledAuth.set(false);
}

/**
 * @brief
*/
void SenderAudioProcessor::addOnSubscribeHandler(corelink::core::network::channel_id_type mControlChannelId,
                           out<corelink::client::corelink_classic_client> mClient)
{
    mCorelinkClient->addOnSubscribe([&](int statusCode) {
        if (statusCode == 0) {
            mThreadPool.addJob([&]() mutable {
                while (nMeasurement < 10000) {
                    mData.clear();
                    std::vector<uint8_t> rtt_data(1024);
                    corelink::utils::json meta;
                    meta.append("packetIndex", nMeasurement.load());
                    meta.append("timestamp", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

                    mCorelinkClient->sendData(mJitterBuffer->getHostId(), std::move(rtt_data), meta);

                    nMeasurement++;
                    std::this_thread::sleep_for(std::chrono::microseconds(1000));
                }
            });
        }
    });
}

/**
 * @brief Destructor for the SenderAudioProcessor class
*/
SenderAudioProcessor::~SenderAudioProcessor()
{
    if (!mLoading.get()) {
        disconnectControlChannel();
    }
    if (mJitterBuffer) {
        mJitterBuffer.reset();
    }
    
}

int SenderAudioProcessor::getNMeasurement()
{
    return nMeasurement.load();
}

bool SenderAudioProcessor::getMDone() const 
{
    return mDone.get();
}

bool SenderAudioProcessor::doesCorelinkClientExist() const
{
    return (mCorelinkClient != nullptr);
}
double SenderAudioProcessor::getAudioSampleRate() const
{
    return mAudioSampleRate;
}
int SenderAudioProcessor::getAudioBufferSize() const
{
    return mAudioBufferSize;
}

/**
 * @brief Sets mStreamInit to true
*/
void SenderAudioProcessor::setStreamInit()
{
    mStreamInit.set(true);
}

/**
 * @brief Returns if stream has been init
*/
bool SenderAudioProcessor::getStreamInit()
{
    return mStreamInit.get();
}

/**
 * @brief Sets the value of Corelink host ID
*/
void SenderAudioProcessor::setHostId(const juce::String &val)
{
    mCorelinkHostId = val;
}

/**
 * @brief Returns Corelink host ID
*/
juce::String SenderAudioProcessor::getHostId()
{
    return mCorelinkHostId;
}

/**
 * @brief Sets the value of audio workspace
*/
void SenderAudioProcessor::setAudioWorkspace(const juce::String &val)
{
    mAudioWorkspace = val.toStdString();;
}
/**
 * @brief Returns the value of audio workspace
*/
juce::String SenderAudioProcessor::getAudioWorkspace()
{
    return mAudioWorkspace;
}

/**
 * @brief Sets the value of audio stream type
*/
void SenderAudioProcessor::setAudioStreamType(const juce::String &val)
{
    mAudioStreamType = val.toStdString();
}
/**
 * @brief Returns the value of audio stream type
*/
juce::String SenderAudioProcessor::getAudioStreamType()
{
    return mAudioStreamType;
}

/**
 * @brief Sets the jitter buffer size
*/
void SenderAudioProcessor::setBufferSize(const juce::String &val)
{
    std::string temp = val.toStdString();
    mJitterBufferSize = std::stoi(temp);
}

/**
 * @brief Returns the jitter buffer size
*/
int SenderAudioProcessor::getBufferSize()
{
    return mJitterBufferSize;
}

/**
 * @brief Prints error log
*/
void SenderAudioProcessor::onError(corelink::core::network::channel_id_type hostId, in<std::string> err)
{
    DBG("Error in host id: " << hostId);
    mDone.set(true);
}
/**
 * @brief Prints log when the channel is connected
*/
void SenderAudioProcessor::onChannelInit(corelink::core::network::channel_id_type hostId)
{
    mDone.set(true);
}
/**
 * @brief Prints log when the channel is disconnected
*/
void SenderAudioProcessor::onChannelUninit(corelink::core::network::channel_id_type hostId)
{
    mDone.set(true);
}

/**
 * @brief Creates the sender processor
*/
void SenderAudioProcessor::createSender(const juce::String& workspace, const juce::String& stream_type)
{
    mCorelinkClient->createSender(workspace, stream_type, [&](int statusCode) {
        mLoading.set(false);
    });
}

/**
 * @brief Returns the name of this audio processor
*/
const juce::String SenderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

/**
 * @brief Returns true if the processor wants MIDI messages
*/
bool SenderAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}
/**
 * @brief Returns true if the processor produces MIDI messages
*/
bool SenderAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}
/**
 * @brief Returns true if this is a MIDI effect plug-in and does no audio processing
*/
bool SenderAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

/**
 * @brief Returns the length of the processor's tail, in seconds
*/
double SenderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

/**
 * @brief Returns the number of preset programs the processor supports
*/
int SenderAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}
/**
 * @brief Returns the number of the currently active program
*/
int SenderAudioProcessor::getCurrentProgram()
{
    return 0;
}
/**
 * @brief Called by the host to change the current program
*/
void SenderAudioProcessor::setCurrentProgram(int index)
{
}
/**
 * @brief Returns the name of a given program
*/
const juce::String SenderAudioProcessor::getProgramName(int index)
{
    return {};
}
/**
 * @brief Called by the host to rename a program
*/
void SenderAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

/**
 * @brief Called before playback starts, to let the processor prepare itself
*/
void SenderAudioProcessor::prepareToPlay(double mSampleRate, int samplesPerBlock)
{
    mAudioBufferSize = samplesPerBlock;
    mAudioSampleRate = mSampleRate;
}

/**
 * @brief Restores the processor's state from a block of data previously created using getStateInformation()
*/
void SenderAudioProcessor::setStateInformation(const void *mData, int sizeInBytes)
{
}
/**
 * @brief The host will call this method when it wants to save the processor's internal state
*/
void SenderAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
}

/**
 * @brief Called after playback has stopped, to let the object free up any resources it no longer needs
*/
void SenderAudioProcessor::releaseResources()
{
}

/**
 * @brief Callback to query if the AudioProcessor supports a specific layout
*/
#ifndef JucePlugin_PreferredChannelConfigurations
bool SenderAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::discreteChannels(NUMBER_CHANNEL))
        return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}
#endif

/**
 * @brief Renders the next block
*/
void SenderAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    int audioBufferSize = buffer.getNumSamples();

    if (!mLoading.get() && totalNumInputChannels >= 2)
    {
        std::async(std::launch::async, &SenderAudioProcessor::sendData, this, std::move(buffer), audioBufferSize, std::ref(mData), totalNumInputChannels);
    }

    // Apply gain to all channels
    buffer.applyGain(0, audioBufferSize, mVolume.get());
}


/**
 * @brief Sends data to Corelink host
*/
void SenderAudioProcessor::sendData(juce::AudioBuffer<float> buffer, int mAudioBufferSize, std::vector<uint8_t> &mData, int numChannels)
{
    if (!mLoading.get())
    {
        corelink::utils::json meta;
        meta.append("counter_value", mPacketCounter);
        meta.append("num_channel", numChannels);

        meta.append("timestamp", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

        mData.clear();
        mData.resize(sizeof(float) * buffer.getNumSamples() * numChannels);

        auto des = mData.data();
        for (int i = 0; i < numChannels; i++) {
            std::memcpy(des, buffer.getReadPointer(i), sizeof(float) * mAudioBufferSize);
            des += sizeof(float) * mAudioBufferSize;
        }
        // Send data
        if (mData.size() > 0)
        {
            mCorelinkClient->sendData(mCorelinkClient->mHostId, mData, meta);
        }

        // Increment packet counter with current buffer size
        mPacketCounter += mAudioBufferSize;
        mPacketCounter = mPacketCounter % (150 * mAudioBufferSize);
    }
}

/**
 * @brief Swaps the objects
*/
template <class T>
void SenderAudioProcessor::swapMove(T &a, T &b)
{
    T tmp{std::move(a)};
    a = std::move(b);
    b = std::move(tmp);
}

/**
 * @brief Returns true if the processor can create an editor component
*/
bool SenderAudioProcessor::hasEditor() const
{
    return true;
}
/**
 * @brief Creates the processor's GUI
*/
juce::AudioProcessorEditor *SenderAudioProcessor::createEditor()
{
    return new SenderAudioProcessorEditor(*this);
}


/**
 * @brief Create plugin filter
*/
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new SenderAudioProcessor();
}

/**
 * @brief Sets the volume of the sent audio
*/
void SenderAudioProcessor::setVolume(float newVolume)
{
    mVolume.set(newVolume);
}
/**
 * @brief Returns the volume of the sent audio
*/
float SenderAudioProcessor::getVolume()
{
    return mVolume.get();
}
