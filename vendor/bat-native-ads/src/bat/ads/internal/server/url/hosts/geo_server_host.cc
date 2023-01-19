/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/url/hosts/geo_server_host.h"

#include <ostream>

#include "base/notreached.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/flag_manager.h"

namespace ads {

namespace {

constexpr char kProductionHost[] = "https://geo.ads.brave.com";
constexpr char kStagingHost[] = "https://geo.ads.bravesoftware.com";

}  // namespace

std::string GeoServerHost::Get() const {
  const EnvironmentType environment_type =
      FlagManager::GetInstance()->GetEnvironmentType();

  switch (environment_type) {
    case EnvironmentType::kProduction: {
      return kProductionHost;
    }

    case EnvironmentType::kStaging: {
      return kStagingHost;
    }
  }

  NOTREACHED() << "Unexpected value for EnvironmentType: "
               << static_cast<int>(environment_type);
  return kStagingHost;
}

}  // namespace ads
