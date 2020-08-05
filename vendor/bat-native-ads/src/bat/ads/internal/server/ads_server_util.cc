/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/ads_server_util.h"

#include "bat/ads/ads.h"

namespace ads {
namespace server {

namespace {

const char kProductionDomain[] = "https://ads-serve.brave.com";
const char kStagingDomain[] = "https://ads-serve.bravesoftware.com";
const char kDevelopmentDomain[] = "https://ads-serve.brave.software";

}  // namespace

std::string GetDomain() {
  switch (_environment) {
    case Environment::PRODUCTION: {
      return kProductionDomain;
    }

    case Environment::STAGING: {
      return kStagingDomain;
    }

    case Environment::DEVELOPMENT: {
      return kDevelopmentDomain;
    }
  }
}

}  // namespace server
}  // namespace ads
