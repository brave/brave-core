/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_waiter.h"

#include "brave/components/brave_ads/core/browser/service/ads_service.h"

namespace brave_ads {

AdsServiceWaiter::AdsServiceWaiter(AdsService& ads_service)
    : ads_service_(ads_service) {
  ads_service_->AddObserver(this);
}

AdsServiceWaiter::~AdsServiceWaiter() {
  ads_service_->RemoveObserver(this);
}

void AdsServiceWaiter::WaitForOnDidInitializeAdsService() {
  on_did_initialize_ads_service_run_loop_.Run();
}

void AdsServiceWaiter::WaitForOnDidShutdownAdsService() {
  on_did_shutdown_ads_service_run_loop_.Run();
}

void AdsServiceWaiter::WaitForOnDidClearAdsServiceData() {
  on_did_clear_ads_service_data_run_loop_.Run();
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceWaiter::OnDidInitializeAdsService() {
  on_did_initialize_ads_service_run_loop_.Quit();
}

void AdsServiceWaiter::OnDidShutdownAdsService() {
  on_did_shutdown_ads_service_run_loop_.Quit();
}

void AdsServiceWaiter::OnDidClearAdsServiceData() {
  on_did_clear_ads_service_data_run_loop_.Quit();
}

}  // namespace brave_ads
