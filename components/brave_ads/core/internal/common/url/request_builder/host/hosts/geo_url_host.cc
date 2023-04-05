/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/geo_url_host.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"

namespace brave_ads {

namespace {

constexpr char kProductionHost[] = "https://geo.ads.brave.com";
constexpr char kStagingHost[] = "https://geo.ads.bravesoftware.com";

}  // namespace

std::string GeoUrlHost::Get() const {
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

}  // namespace brave_ads
