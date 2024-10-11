/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_SERVICE_WAITER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_SERVICE_WAITER_H_

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "brave/components/brave_ads/browser/ads_service_observer.h"

namespace brave_ads {

class AdsService;

// This class waits for the ads service to be initialized.
class AdsServiceWaiter : public AdsServiceObserver {
 public:
  explicit AdsServiceWaiter(AdsService* const ads_service);
  ~AdsServiceWaiter() override;

  AdsServiceWaiter(const AdsServiceWaiter&) = delete;
  AdsServiceWaiter& operator=(const AdsServiceWaiter&) = delete;

  void Wait();

 private:
  // AdsServiceObserver:
  void OnAdsServiceInitialized() override;

  const raw_ptr<AdsService> ads_service_;  // not owned.

  base::RunLoop run_loop_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TEST_ADS_SERVICE_WAITER_H_
