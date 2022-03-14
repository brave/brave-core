/*************************************************************************************************
 * Copyright (c) 2022 The Flower Authors.
 *
 * @file start.h
 *
 * @brief Create a gRPC channel to connect to the server and enable message
 *communication
 *
 * @author Lekang Jiang
 *
 * @version 1.0
 *
 * @date 06/09/2021
 *
 *************************************************************************************************/

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_START_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_START_H_

#include <string>

#include "brave/third_party/flower/src/cc/flwr/include/client.h"
#include "brave/third_party/flower/src/cc/flwr/include/message_handler.h"
#include "components/grpc_support/bidirectional_stream.h"

using flower::transport::ClientMessage;
using flower::transport::FlowerService;
using flower::transport::ServerMessage;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::Status;

#define GRPC_MAX_MESSAGE_LENGTH 536870912  //  == 512 * 1024 * 1024

namespace flower {

/**
 * @brief Start a C++ Flower Client which connects to a gRPC server
 * @param  server_address The IPv6 address of the server. If the Flower server
 * runs on the same machine on port 8080, then `server_address` would be
 * `"[::]:8080"`.
 *
 *         client
 *                        An implementation of the abstract base class
 * `flwr::Client`
 *
 *         grpc_max_message_length
 *                        int (default: 536_870_912, this equals 512MB).
 *                        The maximum length of gRPC messages that can be
 * exchanged with the Flower server. The default should be sufficient for most
 * models. Users who train very large models might need to increase this value.
 * Note that the Flower server needs to be started with the same value (see
 * `flwr.server.start_server`), otherwise it will not know about the increased
 * limit and block larger messages.
 *
 */

class ClientRunner final: public grpc_support::BidirectionalStream::Delegate {

public:
 ClientRunner(const std::string& server_endpoint,
                      flwr::Client* client,
                      int grpc_max_message_length);
 ~ClientRunner();
 ClientRunner(const ClientRunner&) = delete;
 ClientRunner& operator=(const ClientRunner&) = delete;

 void Start();
 
 // Delegate implementation
 void OnStreamReady();
 void OnHeadersReceived(
    const spdy::Http2HeaderBlock& response_headers,
    const char* negotiated_protocol);
 void OnDataRead(char* data, int size);
 void OnDataSent(const char* data);
 void OnTrailersReceived(const spdy::Http2HeaderBlock& trailers);
 void OnSucceeded();
 void OnFailed(int error);
 void OnCanceled();

private:
 grpc_support::BidirectionalStream* bidirectional_stream_;
 flwr::Client* federated_client_;
 const std::string& server_endpoint_;
};

} //  namespace flower

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_START_H_
