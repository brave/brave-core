/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace endpoint {

PromotionServer::PromotionServer(RewardsEngineImpl& engine)
    : get_available_(engine),
      post_creds_(engine),
      get_signed_creds_(engine),
      post_clobbered_claims_(engine),
      post_bat_loss_(engine),
      post_captcha_(engine),
      get_captcha_(engine),
      put_captcha_(engine),
      post_safetynet_(engine),
      put_safetynet_(engine),
      post_devicecheck_(engine),
      put_devicecheck_(engine),
      post_suggestions_(engine),
      post_suggestions_claim_(engine) {}

PromotionServer::~PromotionServer() = default;

}  // namespace endpoint
}  // namespace brave_rewards::internal
