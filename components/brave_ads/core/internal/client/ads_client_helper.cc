/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

// static
AdsClient* AdsClientHelper::GetInstance() {
  AdsClient* ads_client = GlobalState::GetInstance()->GetAdsClient();
  CHECK(ads_client);
  return ads_client;
}

// static
bool AdsClientHelper::HasInstance() {
  return GlobalState::HasInstance();
}

// static
void AdsClientHelper::AddObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);

  GetInstance()->AddObserver(observer);
}

// static
void AdsClientHelper::RemoveObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);

  GetInstance()->RemoveObserver(observer);
}

}  // namespace brave_ads
