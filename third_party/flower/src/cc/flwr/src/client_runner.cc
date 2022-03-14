/* Copyright (c) 2022 The Flower Authors. */

#include "brave/third_party/flower/src/cc/flwr/include/client_runner.h"

#include "base/time/time.h"
#include "net/url_request/url_request_context_getter.h"

namespace flower {
  
ClientRunner::ClientRunner(
  const std::string& server_endpoint,
  flwr::Client* client,
  int grpc_max_message_length)
  : federated_client_(client),
    server_endpoint_(server_endpoint) {
    DCHECK(federated_client_);

    net::URLRequestContextGetter url_request_context_getter({});
    grpc_support::BidirectionalStream* bidirectional_stream_(
      new grpc_support::BidirectionalStream(url_request_context_getter, this));
}

ClientRunner::~ClientRunner() {}

void ClientRunner::Start() {
  DCHECK(federated_client_);

  bidirectional_stream_->Start(
    &server_endpoint_[0],
    /*priority*/ 3,
    "GET",
    {}, // headers
    false
  );

  // grpc::ChannelArguments args;
  // args.SetMaxReceiveMessageSize(grpc_max_message_length_);
  // args.SetMaxSendMessageSize(grpc_max_message_length_);
  // args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, true);

  // int sleep_duration = 0;
  // bool is_connected = true;
  // while (federated_client_->is_communicating() && is_connected) {
  //   // Establish an insecure gRPC connection to a gRPC server
  //   std::shared_ptr<Channel> channel = grpc::CreateCustomChannel(
  //       server_address, grpc::InsecureChannelCredentials(), args); 
  //   // std::cout << "Created channel on " << server_address << std::endl;
  //   // Create stub
  //   std::unique_ptr<FlowerService::Stub> stub_ =
  //       FlowerService::NewStub(channel);
  //   // Read and write messages
  //   ClientContext context;
  //   std::shared_ptr<ClientReaderWriter<ClientMessage, ServerMessage>>
  //       reader_writer(stub_->Join(&context));
  //   ServerMessage sm;
  //   while (reader_writer->Read(&sm)) { // TODO(lminto): Two objects - read and write
  //     std::tuple<ClientMessage, int, bool> receive = handle(federated_client_, sm);
  //     sleep_duration = std::get<1>(receive);
  //     reader_writer->Write(std::get<0>(receive));
  //     if (std::get<2>(receive) == false) {
  //       break;
  //     }
  //   }
  //   reader_writer->WritesDone();

  //   // Check connection status
  //   Status status = reader_writer->Finish();

  //   if (sleep_duration == 0) {
  //     std::cout << "Disconnect and shut down." << std::endl;
  //     is_connected = false;
  //   } else {
  //     std::cout << "Disconnect, then re-establish connection after"
  //               << sleep_duration << "second(s)" << std::endl;
  //     // std::this_thread::sleep_for(base::Time::Milliseconds()); USER TIMER HERE INSTEAD
  //   }
  // }

  // std::cout << "Client is not communicating.";
}

void ClientRunner::OnStreamReady() {
  
}

void ClientRunner::OnHeadersReceived(
    const spdy::Http2HeaderBlock& response_headers,
    const char* negotiated_protocol) {
  
}

void ClientRunner::OnDataRead(char* data, int size) {
  
}

void ClientRunner::OnDataSent(const char* data) {
  
}

void ClientRunner::OnTrailersReceived(const spdy::Http2HeaderBlock& trailers) {
  
}

void ClientRunner::OnSucceeded() {
  
}

void ClientRunner::OnFailed(int error) {
  
}

void ClientRunner::OnCanceled() {
  
}

} // namespace flower
