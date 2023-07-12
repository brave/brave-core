/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"

#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (_environment) {
    case mojom::Environment::DEVELOPMENT:
      url = BUILDFLAG(REWARDS_GRANT_DEV_ENDPOINT);
      break;
    case mojom::Environment::STAGING:
      url = BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT);
      break;
    case mojom::Environment::PRODUCTION:
      url = BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT);
      break;
  }

  return url + path;
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
