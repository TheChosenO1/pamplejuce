#pragma once

#define CORELINK_USE_TCP
#define CORELINK_USE_CONCURRENT_COUNTER
#define CORELINK_USE_CONCURRENT_QUEUE
#define CORELINK_ENABLE_STRING_UTIL_FUNCTIONS

#include "corelink_all.hpp"

template<typename t> using in = corelink::in<t>;
template<typename t> using out = corelink::out<t>;

#define RTT_MEASUREMENTS 10000
#define RTT_PACKET_SIZE 1000

class JitterBuffer{
public:
    JitterBuffer(
        corelink::client::corelink_classic_client& client,
        corelink::core::network::channel_id_type& controlChannelId,
        std::string& workspace,
        std::string& streamType,
        std::string& username
    );

    JitterBuffer();
    ~JitterBuffer();

    corelink::core::network::channel_id_type getHostId();
    corelink::core::network::channel_id_type getStreamId();
    void setupSender();
    void updateEstimatedJitter(int newTransitTime, int index);
    bool checkJitterBufferReady();
    int getAverageJitter();
private:
    corelink::client::corelink_classic_client* mClient;
    corelink::core::network::channel_id_type* mControlChannelId;
    corelink::core::network::channel_id_type mHostId;
    corelink::core::network::channel_id_type mStreamId;
    std::string* mWorkspace;
    std::string* mStreamType;
    std::string* mUsername;
    corelink::utils::json meta;
    std::vector<uint8_t> mData;

    int lastTransitTime = -1;
    int interArrivalTime = -1;
    int jitter = 0;
    int nJitterPackets = 0;
    int totalJitter = 0;
    int averageJitter;
    bool mIsJitterBufferReady = false;
};
