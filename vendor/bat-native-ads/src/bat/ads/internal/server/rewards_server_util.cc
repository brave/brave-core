/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/rewards_server_util.h"

#include "bat/ads/ads.h"

namespace ads {
namespace rewards {
namespace server {

namespace {

const char kProductionDomain[] = "https://grant.rewards.brave.com";
const char kStagingDomain[] = "https://grant.rewards.bravesoftware.com";
const char kDevelopmentDomain[] = "https://grant.rewards.brave.software";

}  // namespace

std::string GetHost() {
  switch (g_environment) {
    case mojom::Environment::kProduction: {
      return kProductionDomain;
    }

    case mojom::Environment::kStaging: {
      return kStagingDomain;
    }

    case mojom::Environment::kDevelopment: {
      return kDevelopmentDomain;
    }
  }
}

}  // namespace server
}  // namespace rewards
}  // namespace ads
