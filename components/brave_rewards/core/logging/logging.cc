/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/logging/logging.h"

namespace brave_rewards::internal {

mojom::RewardsEngineClient* g_rewards_engine_client = nullptr;  // NOT OWNED

void set_client_for_logging(mojom::RewardsEngineClient* client) {
  g_rewards_engine_client = client;
}

void Log(const char* file,
         const int line,
         const int verbose_level,
         const std::string& message) {
  if (!g_rewards_engine_client) {
    return;
  }

  g_rewards_engine_client->Log(file, line, verbose_level, message);
}

}  // namespace brave_rewards::internal
