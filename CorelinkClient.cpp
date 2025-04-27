#include "CorelinkClient.h"
#include "PluginProcessor.h"

CorelinkClient::CorelinkClient() {
    std::cout << "Corelink Client Contructor called." << std::endl;
}

CorelinkClient::~CorelinkClient() {
    std::cout << "Corelink Client Destructor called." << std::endl;
}

bool CorelinkClient::initProtocols() {
    return mClient.init_protocols();
}

void CorelinkClient::addControlChannel(SenderAudioProcessor* senderAudioProcessor) {
    auto controlChannelId = mClient.add_control_channel(
        mInfo.protocol,
        mInfo.hostname,
        mInfo.port_number,
        mInfo.client_certificate_path,
        std::bind(&SenderAudioProcessor::onError, senderAudioProcessor, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SenderAudioProcessor::onChannelInit, senderAudioProcessor, std::placeholders::_1),
        std::bind(&SenderAudioProcessor::onChannelUninit, senderAudioProcessor, std::placeholders::_1));

    setControlChannelId(controlChannelId);
}

void CorelinkClient::authenticate(const juce::String& username, const juce::String& password, const std::function<void(int)>& cb) {
    auto val = std::make_shared<corelink::client::request_response::requests::authenticate_client_request>(username.toStdString(), password.toStdString());

    mClient.request(mControlChannelId,
                   corelink::client::corelink_functions::authenticate,
                   val,
                   [cb](corelink::core::network::channel_id_type hostId,
                       in<std::string> msg,
                       in<std::shared_ptr<corelink::client::request_response::responses::corelink_server_response_base>> response)
                   {
                    cb(response->status_code);
                   });
}

void CorelinkClient::disconnectChannel(std::vector<corelink::core::network::channel_id_type> streamIDs, const std::function<void(int, corelink::core::network::channel_id_type)> cb) {
    auto disconnectRequest = std::make_shared<corelink::client::request_response::requests::disconnect_streams_request>();
    for (corelink::core::network::channel_id_type id : streamIDs) {
        disconnectRequest->streamIDs.push_back(id);
    }
    
    mClient.request(mControlChannelId,
                    corelink::client::corelink_functions::disconnect,
                    disconnectRequest,
                    [cb](corelink::core::network::channel_id_type channel_id,
                         in<std::string>,
                         in<std::shared_ptr<corelink::client::request_response::responses::corelink_server_response_base>> response)
                    {
                        cb(response->status_code, channel_id);
                    }
    );
}



void CorelinkClient::addOnSubscribe(const std::function<void(int)>& cb) {
    mClient.request(
        mControlChannelId,
        corelink::client::corelink_functions::server_callback_on_subscribed, nullptr,
        [cb](corelink::core::network::channel_id_type hostId, in<std::string>,
            in<std::shared_ptr<corelink::client::request_response::responses::corelink_server_response_base>> response)
        {
            cb(response->status_code);
        }
    );
}

void CorelinkClient::createSender(const juce::String& workspace, const juce::String& stream_type, const std::function<void(int)> cb) {
    auto request =
    std::make_shared<corelink::client::request_response::requests::modify_sender_stream_request>(corelink::core::network::constants::protocols::udp);

    request->client_certificate_path = mCertPath;
    request->alert                   = true;
    request->echo                    = true;
    request->workspace               = workspace.toStdString();
    request->stream_type             = stream_type.toStdString();

    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    request->meta                    = "{ \"username\": \"" + mUsername + "\",\n"
                                       "  \"timestamp\": \"" + std::to_string(ts) + "\",\n"
                                       "  \"type\": \"audio\" }";

    request->on_error                = [](
                            corelink::core::network::channel_id_type hostId,
                            in<std::string> err)
    {
        DBG("Error while sending data on the data channel: " << err);
    };
    request->
        on_send = [](corelink::core::network::channel_id_type hostId, size_t bytes_sent)
    {
    };
    request->
        on_init = [&](corelink::core::network::channel_id_type hostId)
    {
    };

    mClient.request(
        mControlChannelId,
        corelink::client::corelink_functions::create_sender,
        request,
        [&, cb](corelink::core::network::channel_id_type hostId,
            in<std::string> /*msg*/,
            in<std::shared_ptr<corelink::client::request_response::responses::corelink_server_response_base>> response)
        {
            corelink::utils::json receiver_response(response->message);
            mStreamId = receiver_response.get_int("streamID");
            
            mHostId = hostId;
            cb(response->status_code);
    });
}

void CorelinkClient::sendData(corelink::core::network::channel_id_type hostId, std::vector<uint8_t> mData, corelink::utils::json meta) {
    mClient.send_data(hostId, std::move(mData), std::move(meta));
}

void CorelinkClient::setControlChannelId(corelink::core::network::channel_id_type controlChannelId){
    mControlChannelId = controlChannelId;
}

void CorelinkClient::setInfo(const juce::String& hostId, const juce::String& username) {
    mInfo.set_hostname(hostId.toStdString());
    mInfo.set_port_number(20010);
    mUsername = username.toStdString();
}
