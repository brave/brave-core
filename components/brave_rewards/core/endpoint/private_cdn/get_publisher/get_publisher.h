/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// GET /publishers/prefixes/{prefix}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_NOT_FOUND (404)
//
// Response body:
// See
// https://github.com/brave/brave-core/blob/master/components/brave_rewards/core/publisher/protos/channel_response.proto

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace private_cdn {

using GetPublisherCallback =
    std::function<void(const mojom::Result result,
                       mojom::ServerPublisherInfoPtr info)>;

class GetPublisher {
 public:
  explicit GetPublisher(RewardsEngineImpl& engine);
  ~GetPublisher();

  void Request(const std::string& publisher_key,
               const std::string& hash_prefix,
               GetPublisherCallback callback);

 private:
  std::string GetUrl(const std::string& hash_prefix);

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body,
                          const std::string& publisher_key,
                          mojom::ServerPublisherInfo* info);

  void OnRequest(const std::string& publisher_key,
                 GetPublisherCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_GET_PUBLISHER_GET_PUBLISHER_H_
