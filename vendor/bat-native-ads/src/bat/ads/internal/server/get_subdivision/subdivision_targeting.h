/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_
#define BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_

#include <string>

#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/mojom.h"

namespace ads {

class AdsImpl;

class SubdivisionTargeting {
 public:
  SubdivisionTargeting(
      AdsImpl* ads);

  ~SubdivisionTargeting();

  bool ShouldAllowForLocale(
      const std::string& locale) const;

  bool IsDisabled() const;

  void MaybeFetchForLocale(
      const std::string& locale);

  void MaybeFetchForCurrentLocale();

  std::string GetAdsSubdivisionTargetingCode() const;

 private:
  Timer timer_;
  BackoffTimer retry_timer_;

  bool IsSupportedLocale(
      const std::string& locale) const;

  void MaybeAllowForLocale(
      const std::string& locale);

  bool ShouldAutomaticallyDetect() const;

  void Fetch();

  void OnFetch(
      const UrlResponse& response);

  bool ParseJson(
      const std::string& json);

  void Retry();

  void FetchAfterDelay();

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_SUBDIVISION_TARGETING_H_
