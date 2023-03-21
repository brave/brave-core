/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/logging/logging.h"

namespace ledger {

rewards::mojom::RewardsService* g_rewards_service = nullptr;  // NOT OWNED

void set_ledger_client_for_logging(
    rewards::mojom::RewardsService* rewards_service) {
  DCHECK(rewards_service);
  g_rewards_service = rewards_service;
}

void Log(const char* file,
         const int line,
         const int verbose_level,
         const std::string& message) {
  if (!g_rewards_service) {
    return;
  }

  g_rewards_service->Log(file, line, verbose_level, message);
}

}  // namespace ledger
