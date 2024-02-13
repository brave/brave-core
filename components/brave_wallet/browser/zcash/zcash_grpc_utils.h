/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_

#include <string>
#include <string_view>

#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"

namespace brave_wallet {

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
  virtual bool ProcessMessage(std::string_view message) = 0;

  std::string data_;
  size_t retry_counter_ = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GRPC_UTILS_H_
