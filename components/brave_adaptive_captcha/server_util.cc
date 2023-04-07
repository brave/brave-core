/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/server_util.h"

#include "base/check.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_rewards/core/buildflags.h"

namespace brave_adaptive_captcha {

using brave_rewards::RewardsFlags;

ServerUtil::ServerUtil() = default;
ServerUtil::~ServerUtil() = default;

ServerUtil* ServerUtil::GetInstance() {
  static base::NoDestructor<ServerUtil> instance;
  return instance.get();
}

std::string ServerUtil::GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetHost() + path;
}

void ServerUtil::SetServerHostForTesting(const std::string& host) {
  server_host_ = host;
}

std::string ServerUtil::GetHost() {
  if (!server_host_.empty()) {
    return server_host_;
  }

  const auto& flags = RewardsFlags::ForCurrentProcess();
  switch (flags.environment.value_or(RewardsFlags::Environment::kProduction)) {
    case RewardsFlags::Environment::kDevelopment:
    case RewardsFlags::Environment::kStaging:
      return BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT);
    case RewardsFlags::Environment::kProduction:
      return BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT);
  }
}

}  // namespace brave_adaptive_captcha
