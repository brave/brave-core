/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/server_util.h"

#include "base/check.h"
#include "bat/ledger/buildflags.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_adaptive_captcha {

namespace {

using brave_rewards::RewardsFlags;

std::string GetHost() {
  const auto& flags = RewardsFlags::ForCurrentProcess();
  switch (flags.environment.value_or(RewardsFlags::Environment::kProduction)) {
    case RewardsFlags::Environment::kDevelopment:
    case RewardsFlags::Environment::kStaging:
      return BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT);
    case RewardsFlags::Environment::kProduction:
      return BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT);
  }
}

}  // namespace

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetHost() + path;
}

}  // namespace brave_adaptive_captcha
