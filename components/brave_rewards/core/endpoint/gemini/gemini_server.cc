/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"

#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::endpoint {

GeminiServer::GeminiServer(RewardsEngine& engine)
    : post_account_(engine),
      post_balance_(engine),
      post_oauth_(engine),
      post_recipient_id_(engine) {}

GeminiServer::~GeminiServer() = default;

}  // namespace brave_rewards::internal::endpoint
