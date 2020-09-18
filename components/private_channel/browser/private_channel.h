/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/values.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
}

namespace private_channel {

class PrivateChannel {
 public:
  explicit PrivateChannel(std::string referral_code);
  ~PrivateChannel();

  void PerformReferralAttestation();

 private:
  void FetchMetadataPrivateChannelServer();

  void OnPrivateChannelMetaLoadComplete(
      std::unique_ptr<std::string> response_body);

  void FirstRoundProtocol(std::string server_pubkey);

  void OnPrivateChannelFirstRoundLoadComplete(
      std::string client_sks,
      std::string id,
      int input_size,
      std::unique_ptr<std::string> response_body);

  void SecondRoundProtocol(const std::string& encrypted_input,
                           std::string client_sks,
                           std::string id,
                           int input_size);

  void OnPrivateChannelSecondRoundLoadComplete(
      std::unique_ptr<std::string> response_body);

  std::unique_ptr<network::SimpleURLLoader> http_loader_;
  std::string referral_code_;
};

}  // namespace private_channel

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_
