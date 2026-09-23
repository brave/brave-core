/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_TEST_ADS_SERVICE_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_TEST_ADS_SERVICE_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsServiceObserverMock : public AdsServiceObserver {
 public:
  AdsServiceObserverMock();

  AdsServiceObserverMock(const AdsServiceObserverMock&) = delete;
  AdsServiceObserverMock& operator=(const AdsServiceObserverMock&) = delete;

  ~AdsServiceObserverMock() override;

  MOCK_METHOD(void, OnAdsServiceIneligibleToStart, (), (override));
  MOCK_METHOD(void, OnDidInitializeAdsService, (), (override));
  MOCK_METHOD(void, OnDidShutdownAdsService, (), (override));
  MOCK_METHOD(void, OnDidClearAdsServiceData, (), (override));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_TEST_ADS_SERVICE_OBSERVER_MOCK_H_
