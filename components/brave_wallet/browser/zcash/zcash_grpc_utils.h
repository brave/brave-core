/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_

#include <string>
#include <string_view>

#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// Resolves serialized protobuf from length-prefixed string
absl::optional<std::string> ResolveSerializedMessage(
    const std::string& grpc_response_body);

// Prefixes provided serialized protobuf with compression byte and 4 bytes of
// message size. See
// https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
std::string GetPrefixedProtobuf(const std::string& serialized_proto);

// Handles a stream of GRPC objects
class GRrpcMessageStreamHandler
    : public network::SimpleURLLoaderStreamConsumer {
 public:
  GRrpcMessageStreamHandler();
  ~GRrpcMessageStreamHandler() override;

  void OnDataReceived(std::string_view string_piece,
                      base::OnceClosure resume) override;
  void OnRetry(base::OnceClosure start_retry) override;

 private:
  virtual bool ProcessMessage(const std::string& message) = 0;

  absl::optional<size_t> data_to_read_;
  std::string data_;
  size_t retry_counter_ = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_
