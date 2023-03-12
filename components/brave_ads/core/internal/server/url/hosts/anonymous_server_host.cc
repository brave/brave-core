/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/server/url/hosts/anonymous_server_host.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"

namespace ads {

namespace {

constexpr char kProductionHost[] = "https://anonymous.ads.brave.com";
constexpr char kStagingHost[] = "https://anonymous.ads.bravesoftware.com";

}  // namespace

std::string AnonymousServerHost::Get() const {
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
