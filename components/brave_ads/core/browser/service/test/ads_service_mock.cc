/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"

#include "brave/components/brave_ads/core/browser/service/ads_service_observer.h"

namespace brave_ads {

AdsServiceMock::AdsServiceMock() : AdsService(/*delegate=*/nullptr) {}

AdsServiceMock::~AdsServiceMock() = default;

base::WeakPtr<AdsService> AdsServiceMock::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void AdsServiceMock::NotifyObserversOnDidShutdownAdsServiceForTesting() {
  observers_.Notify(&AdsServiceObserver::OnDidShutdownAdsService);
}

void AdsServiceMock::NotifyObserversOnDidInitializeAdsServiceForTesting() {
  observers_.Notify(&AdsServiceObserver::OnDidInitializeAdsService);
}

}  // namespace brave_ads
