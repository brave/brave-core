/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/ads_factory_impl.h"

#include "base/files/file_path.h"
#include "brave/components/brave_ads/core/public/ads.h"

namespace brave_ads {

AdsFactoryImpl::AdsFactoryImpl() = default;

AdsFactoryImpl::~AdsFactoryImpl() = default;

std::unique_ptr<Ads> AdsFactoryImpl::CreateAds(
    AdsClient& ads_client,
    const base::FilePath& database_path) {
  return Ads::CreateInstance(ads_client, database_path);
}

}  // namespace brave_ads
