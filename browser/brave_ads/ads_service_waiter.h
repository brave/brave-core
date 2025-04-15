/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_
#define BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_

#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"

namespace brave_ads {

class AdsService;

namespace test {

class AdsServiceWaiter final : public AdsServiceObserver {
 public:
  explicit AdsServiceWaiter(AdsService* ads_service);

  AdsServiceWaiter(const AdsServiceWaiter&) = delete;
  AdsServiceWaiter& operator=(const AdsServiceWaiter&) = delete;

  ~AdsServiceWaiter() override;

  void WaitForOnDidInitializeAdsService();
  void WaitForOnDidShutdownAdsService();
  void WaitForOnDidClearAdsServiceData();

 private:
  // AdsServiceObserver:
  void OnDidInitializeAdsService() override;
  void OnDidShutdownAdsService() override;
  void OnDidClearAdsServiceData() override;

  base::RunLoop on_did_initialize_ads_service_run_loop_;
  base::RunLoop on_did_shutdown_ads_service_run_loop_;
  base::RunLoop on_did_clear_ads_service_data_run_loop_;

  base::ScopedObservation<AdsService, AdsServiceObserver> observation_{this};
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_WAITER_H_
