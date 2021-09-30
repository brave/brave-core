// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_ads/brave_ads_host_android.h"

#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_ads {

BraveAdsHostAndroid::BraveAdsHostAndroid(Profile* profile) : profile_(profile) {
  DCHECK(profile_);
}

BraveAdsHostAndroid::~BraveAdsHostAndroid() = default;

void BraveAdsHostAndroid::RequestAdsEnabled(
    RequestAdsEnabledCallback callback) {
  const AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
  if (!ads_service) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(ads_service->IsEnabled());
}

}  // namespace brave_ads
