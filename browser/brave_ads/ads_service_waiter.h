/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_
#define BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_

#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"

namespace brave_ads {

class AdsService;

namespace test {

// Observes `AdsService` and waits for lifecycle events in browser tests.
class AdsServiceWaiter final : public AdsServiceObserver {
 public:
  explicit AdsServiceWaiter(AdsService& ads_service);

  AdsServiceWaiter(const AdsServiceWaiter&) = delete;
  AdsServiceWaiter& operator=(const AdsServiceWaiter&) = delete;

  ~AdsServiceWaiter() override;

  // Blocks until the service notifies ineligibility, or returns immediately if
  // the service was already marked ineligible before observation began.
  void WaitForOnAdsServiceIneligibleToStart();

  // Blocks until the service notifies initialization, or returns immediately if
  // the service was already initialized before observation began.
  void WaitForOnDidInitializeAdsService();

  // Blocks until the service notifies shutdown.
  void WaitForOnDidShutdownAdsService();

  // Blocks until the service notifies data cleared.
  void WaitForOnDidClearAdsServiceData();

 private:
  // AdsServiceObserver:
  void OnAdsServiceIneligibleToStart() override;
  void OnDidInitializeAdsService() override;
  void OnDidShutdownAdsService() override;
  void OnDidClearAdsServiceData() override;

  const raw_ref<AdsService> ads_service_;

  base::RunLoop on_ads_service_ineligible_to_start_run_loop_;
  base::RunLoop on_did_initialize_ads_service_run_loop_;
  base::RunLoop on_did_shutdown_ads_service_run_loop_;
  base::RunLoop on_did_clear_ads_service_data_run_loop_;

  base::ScopedObservation<AdsService, AdsServiceObserver> observation_{this};
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_
