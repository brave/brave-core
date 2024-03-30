/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_PRIVATE_CDN_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_PRIVATE_CDN_SERVER_H_

#include "brave/components/brave_rewards/core/endpoint/private_cdn/get_publisher/get_publisher.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint {

class PrivateCDNServer {
 public:
  explicit PrivateCDNServer(RewardsEngine& engine);
  ~PrivateCDNServer();

  private_cdn::GetPublisher& get_publisher() { return get_publisher_; }

 private:
  private_cdn::GetPublisher get_publisher_;
};

}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PRIVATE_CDN_PRIVATE_CDN_SERVER_H_
