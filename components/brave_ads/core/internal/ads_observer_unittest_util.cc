/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_observer_unittest_util.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"

namespace brave_ads {

AdsObserverMock* AddAdsObserverMock() {
  std::unique_ptr<AdsObserverMock> ads_observer_mock =
      std::make_unique<AdsObserverMock>();
  AdsObserverMock* value = ads_observer_mock.get();
  AdsNotifierManager::GetInstance().AddObserver(std::move(ads_observer_mock));
  return value;
}

}  // namespace brave_ads
