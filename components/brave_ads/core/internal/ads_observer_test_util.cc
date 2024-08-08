/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/ads_observer_mock.h"
#include "brave/components/brave_ads/core/internal/ads_observer_test_util.h"

namespace brave_ads::test {

AdsObserverMock* AddAdsObserverMock() {
  std::unique_ptr<AdsObserverMock> ads_observer_mock =
      std::make_unique<AdsObserverMock>();

  AdsObserverMock* const ads_observer_mock_ptr = &*ads_observer_mock;

  // `AdsNotifierManager` takes ownership of `ads_observer_mock`.
  AdsNotifierManager::GetInstance().AddObserver(std::move(ads_observer_mock));

  return ads_observer_mock_ptr;
}

}  // namespace brave_ads::test
