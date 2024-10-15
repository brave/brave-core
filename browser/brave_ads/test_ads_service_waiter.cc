/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/test_ads_service_waiter.h"

#include "base/check.h"
#include "brave/components/brave_ads/browser/ads_service.h"

namespace brave_ads {

AdsServiceWaiter::AdsServiceWaiter(AdsService* const ads_service)
    : ads_service_(ads_service) {
  CHECK(ads_service_);

  ads_service_->AddObserver(this);
}

AdsServiceWaiter::~AdsServiceWaiter() {
  ads_service_->RemoveObserver(this);
}

void AdsServiceWaiter::Wait() {
  run_loop_.Run();
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceWaiter::OnAdsServiceInitialized() {
  run_loop_.Quit();
}

}  // namespace brave_ads
