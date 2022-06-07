#include "start.h"

#include <chrono>
#include <thread>

start::start() {}

void start::start_client(std::string server_address,
                         flwr::Client* client,
                         int grpc_max_message_length) {
  // Set channel parameters
  grpc::ChannelArguments args;
  args.SetMaxReceiveMessageSize(grpc_max_message_length);
  args.SetMaxSendMessageSize(grpc_max_message_length);
  args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, true);

  int sleep_duration = 0;
  bool is_connected = true;
  while (client->is_communicating() && is_connected) {
    // Establish an insecure gRPC connection to a gRPC server
    std::shared_ptr<Channel> channel = grpc::CreateCustomChannel(
        server_address, grpc::InsecureChannelCredentials(), args);
    std::cout << "Created channel on " << server_address << std::endl;
    // Create stub
    std::unique_ptr<FlowerService::Stub> stub_ =
        FlowerService::NewStub(channel);
    // Read and write messages
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<ClientMessage, ServerMessage>>
        reader_writer(stub_->Join(&context));
    ServerMessage sm;
    while (reader_writer->Read(&sm)) {
      std::tuple<ClientMessage, int, bool> receive = handle(client, sm);
      sleep_duration = std::get<1>(receive);
      reader_writer->Write(std::get<0>(receive));
      if (std::get<2>(receive) == false) {
        break;
      }
    }
    reader_writer->WritesDone();

    // Check connection status
    Status status = reader_writer->Finish();

    if (sleep_duration == 0) {
      std::cout << "Disconnect and shut down." << std::endl;
      is_connected = false;
    } else {
      std::cout << "Disconnect, then re-establish connection after"
                << sleep_duration << "second(s)" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration * 1000));
    }
  }

  std::cout << "Client is not communicating.";
}
