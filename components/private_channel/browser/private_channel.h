/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_

#include <memory>
#include <string>

#include "brave/components/private_channel/client_private_channel.h"

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "url/gurl.h"

namespace network {
class SimpleURLLoader;
}

namespace private_channel {

class PrivateChannel {
 public:
  PrivateChannel();
  ~PrivateChannel();

  void PerformReferralAttestation(std::string referral_code);

 private:
  void FetchMetadataPrivateChannelServer();

  void OnPrivateChannelMetaLoadComplete(
      std::unique_ptr<std::string> response_body);

  void FirstRoundProtocol(ChallengeArtefacts request_artefacts);

  void OnPrivateChannelFirstRoundLoadComplete(
      std::string client_sks,
      std::string id,
      int input_size,
      std::unique_ptr<std::string> response_body);

  void SecondRoundProtocol(SecondRoundArtefacts request_artefacts);

  void OnPrivateChannelSecondRoundLoadComplete(
      std::unique_ptr<std::string> response_body);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::unique_ptr<network::SimpleURLLoader> http_loader_;
  std::string referral_code_;

  base::WeakPtrFactory<PrivateChannel> weak_factory_;
};

}  // namespace private_channel

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_H_
