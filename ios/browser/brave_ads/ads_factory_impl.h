/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_IMPL_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_IMPL_H_

#include <memory>

#include "brave/ios/browser/brave_ads/ads_factory.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_ads {

class Ads;
class AdsClient;

// Production `AdsFactory` that builds the real `Ads` engine.
class AdsFactoryImpl final : public AdsFactory {
 public:
  AdsFactoryImpl();

  AdsFactoryImpl(const AdsFactoryImpl&) = delete;
  AdsFactoryImpl& operator=(const AdsFactoryImpl&) = delete;

  ~AdsFactoryImpl() override;

  std::unique_ptr<Ads> CreateAds(
      AdsClient& ads_client,
      const base::FilePath& database_path) override;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_IMPL_H_
