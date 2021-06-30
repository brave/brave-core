/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_

#include <string>

#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

namespace ad_targeting {
namespace geographic {

class SubdivisionTargeting {
 public:
  SubdivisionTargeting();

  ~SubdivisionTargeting();

  bool ShouldAllowForLocale(const std::string& locale) const;

  bool IsDisabled() const;

  void MaybeFetchForLocale(const std::string& locale);

  void MaybeFetchForCurrentLocale();

  std::string GetAdsSubdivisionTargetingCode() const;

 private:
  Timer timer_;
  BackoffTimer retry_timer_;

  bool IsSupportedLocale(const std::string& locale) const;

  void MaybeAllowForLocale(const std::string& locale);

  bool ShouldAutoDetect() const;

  void Fetch();

  void OnFetch(const mojom::UrlResponse& url_response);

  bool ParseJson(const std::string& json);

  void Retry();

  void FetchAfterDelay();
};

}  // namespace geographic
}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_GEOGRAPHIC_SUBDIVISION_SUBDIVISION_TARGETING_H_
