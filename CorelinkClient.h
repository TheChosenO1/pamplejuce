#pragma once

#define CORELINK_USE_TCP
#define CORELINK_USE_CONCURRENT_COUNTER
#define CORELINK_USE_CONCURRENT_QUEUE
#define CORELINK_ENABLE_STRING_UTIL_FUNCTIONS

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
#include <cstdint>
class SenderAudioProcessor;

template<typename t> using in = corelink::in<t>;
template<typename t> using out = corelink::out<t>;

class CorelinkClient{
public:
    std::string mUsername;
    std::string mCertPath;
    corelink::client::corelink_client_connection_info mInfo = corelink::client::corelink_client_connection_info(corelink::core::network::constants::protocols::tcp).set_certificate_path(mCertPath);
    corelink::client::corelink_classic_client mClient;
    corelink::core::network::channel_id_type mHostId;
    corelink::core::network::channel_id_type mStreamId;
    corelink::core::network::channel_id_type mControlChannelId;

    CorelinkClient();
    ~CorelinkClient();

    void addControlChannel(SenderAudioProcessor* senderAudioProcessor);
    bool initProtocols();
    void authenticate(const juce::String& username, const juce::String& password, const std::function<void(int)>& cb);
    void disconnectChannel(std::vector<corelink::core::network::channel_id_type> streamIDs, const std::function<void(int, corelink::core::network::channel_id_type)> cb);

    void addOnSubscribe(const std::function<void(int)>& cb);
    void createSender(const juce::String& workspace, const juce::String& stream_type, const std::function<void(int)> cb);
    void sendData(corelink::core::network::channel_id_type hostId, std::vector<uint8_t> mData, corelink::utils::json meta);
    void setInfo(const juce::String& hostId, const juce::String& username);

private:
    corelink::utils::json meta;
    std::vector<uint8_t> mData;

    void setControlChannelId(corelink::core::network::channel_id_type controlChannelId);
};
