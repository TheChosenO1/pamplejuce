#include "JitterBuffer.h"

JitterBuffer::JitterBuffer(
    corelink::client::corelink_classic_client& client,
    corelink::core::network::channel_id_type& controlChannelId,
    std::string& workspace,
    std::string& streamType,
    std::string& username
): mClient(&client), mControlChannelId(&controlChannelId), mWorkspace(&workspace), mStreamType(&streamType), mUsername(&username) {}

void JitterBuffer::setupSender() {
    auto request =
      std::make_shared<corelink::client::request_response::requests::modify_sender_stream_request>(corelink::core::network::constants::protocols::udp);

    request->alert         = true;
    request->echo          = true;
    request->workspace     = *mWorkspace;
    request->stream_type   = *mStreamType;
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    request->meta          = "{ \"username\": \"" + *mUsername + "\",\n"
                           "  \"timestamp\": \"" + std::to_string(ts) + "\",\n"
                           "  \"type\": \"" + *mStreamType + "\" }";
    request->on_init = [&](corelink::core::network::channel_id_type hostId)
    {
      mHostId = hostId;
    };

    mClient->request(
        *mControlChannelId,
        corelink::client::corelink_functions::create_sender,
        request,
        [&](corelink::core::network::channel_id_type hostId,
            in<std::string>,
            in<std::shared_ptr<corelink::client::request_response::responses::corelink_server_response_base>> response)
        {
            corelink::utils::json receiver_response(response->message);
            mStreamId = receiver_response.get_int("streamID");
            mHostId = hostId;
        });
}

JitterBuffer::JitterBuffer() {
  std::cout << "JitterBuffer Constructor" << std::endl;
}

JitterBuffer::~JitterBuffer() {
  std::cout << "JitterBuffer Destructor" << std::endl;
}

corelink::core::network::channel_id_type JitterBuffer::getHostId() {
    return mHostId;
}
corelink::core::network::channel_id_type JitterBuffer::getStreamId() {
    return mStreamId;
}

void JitterBuffer::updateEstimatedJitter(int newTransitTime, int index)
{
    if (lastTransitTime == -1) {
        lastTransitTime = newTransitTime;
    } else {
        interArrivalTime = newTransitTime - lastTransitTime;
        lastTransitTime = newTransitTime;
        jitter = jitter + ((std::abs(interArrivalTime) - jitter) / 16);
        totalJitter += jitter;
    }
    nJitterPackets++;

    if (index == 9999) mIsJitterBufferReady = true;
}

bool JitterBuffer::checkJitterBufferReady() {
    return mIsJitterBufferReady;
}

int JitterBuffer::getAverageJitter() {
    return totalJitter / nJitterPackets;
}
