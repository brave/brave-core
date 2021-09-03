/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/rewards_server_util.h"

#include "bat/ads/ads.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace rewards {
namespace server {

std::string GetHost() {
  switch (g_environment) {
    case mojom::Environment::kProduction: {
      return REWARDS_GRANT_PROD_ENDPOINT;
    }

    case mojom::Environment::kStaging: {
      return REWARDS_GRANT_STAGING_ENDPOINT;
    }

    case mojom::Environment::kDevelopment: {
      return REWARDS_GRANT_DEV_ENDPOINT;
    }
  }
}

}  // namespace server
}  // namespace rewards
}  // namespace ads
