/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/public/ads_observer_interface.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsObserverMock : public AdsObserverInterface {
 public:
  AdsObserverMock();

  AdsObserverMock(const AdsObserverMock&) = delete;
  AdsObserverMock& operator=(const AdsObserverMock&) = delete;

  ~AdsObserverMock() override;

  MOCK_METHOD(void, OnAdRewardsDidChange, ());

  MOCK_METHOD(void, OnBrowserUpgradeRequiredToServeAds, ());

  MOCK_METHOD(void, OnIneligibleWalletToServeAds, ());

  MOCK_METHOD(void, OnRemindUser, (mojom::ReminderType mojom_reminder_type));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_OBSERVER_MOCK_H_
