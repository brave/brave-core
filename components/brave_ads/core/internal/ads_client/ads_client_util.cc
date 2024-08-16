/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

AdsClient* GetAdsClient() {
  CHECK(GlobalState::HasInstance());

  AdsClient* const ads_client = GlobalState::GetInstance()->GetAdsClient();
  CHECK(ads_client);

  return ads_client;
}

}  // namespace brave_ads
