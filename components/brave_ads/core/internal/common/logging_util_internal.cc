/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/logging_util_internal.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

void Log(const char* const file,
         const int line,
         const int verbose_level,
         const std::string& message) {
  if (GlobalState::HasInstance()) {
    GetAdsClient()->Log(file, line, verbose_level, message);
  }
}

}  // namespace brave_ads
