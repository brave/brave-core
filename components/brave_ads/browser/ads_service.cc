/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service.h"

namespace brave_ads {

AdsService::AdsService() = default;

AdsService::~AdsService() = default;

void AdsService::AddObserver(
    AdsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AdsService::RemoveObserver(
    AdsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_ads
