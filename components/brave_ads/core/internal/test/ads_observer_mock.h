/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TEST_ADS_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TEST_ADS_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/public/ads_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsObserverMock : public AdsObserver {
 public:
  AdsObserverMock();

  AdsObserverMock(const AdsObserverMock&) = delete;
  AdsObserverMock& operator=(const AdsObserverMock&) = delete;

  ~AdsObserverMock() override;

  MOCK_METHOD(void, OnAdRewardsDidChange, ());

  MOCK_METHOD(void, OnBrowserUpgradeRequiredToServeAds, ());

  MOCK_METHOD(void, OnIneligibleWalletToServeAds, ());

  MOCK_METHOD(void,
              OnSolveCaptchaToServeAds,
              (const std::string& payment_id, const std::string& captcha_id));

  MOCK_METHOD(void, OnRemindUser, (mojom::ReminderType));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TEST_ADS_OBSERVER_MOCK_H_
