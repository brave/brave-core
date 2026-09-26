/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_H_

#include <memory>

namespace base {
class FilePath;
}  // namespace base

namespace brave_ads {

class Ads;
class AdsClient;

// Seam for constructing the `Ads` engine, letting tests substitute a fake
// engine instead of the real one built by `Ads::CreateInstance`.
class AdsFactory {
 public:
  virtual ~AdsFactory() = default;

  virtual std::unique_ptr<Ads> CreateAds(
      AdsClient& ads_client,
      const base::FilePath& database_path) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_FACTORY_H_
