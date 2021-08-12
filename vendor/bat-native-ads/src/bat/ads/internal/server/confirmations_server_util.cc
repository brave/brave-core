/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/confirmations_server_util.h"

#include "bat/ads/ads.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace confirmations {
namespace server {

namespace {

const char kProductionHost[] = "https://ads-serve.brave.com";
const char kStagingHost[] = "https://ads-serve.bravesoftware.com";
const char kDevelopmentHost[] = "https://ads-serve.brave.software";

}  // namespace

std::string GetHost() {
  switch (g_environment) {
    case mojom::Environment::kProduction: {
      return kProductionHost;
    }

    case mojom::Environment::kStaging: {
      return kStagingHost;
    }

    case mojom::Environment::kDevelopment: {
      return kDevelopmentHost;
    }
  }
}

}  // namespace server
}  // namespace confirmations
}  // namespace ads
