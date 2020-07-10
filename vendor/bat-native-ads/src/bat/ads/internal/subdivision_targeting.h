/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_H_
#define BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_H_

#include <map>
#include <string>

#include "bat/ads/internal/retry_timer.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class AdsClient;

class SubdivisionTargeting {
 public:
  SubdivisionTargeting(
      AdsClient* ads_client);

  ~SubdivisionTargeting();

  bool ShouldAllowAdsSubdivisionTargeting(
      const std::string& locale) const;

  bool IsDisabled() const;

  void MaybeFetch(
      const std::string& locale);

  std::string GetAdsSubdivisionTargetingCode() const;

 private:
  Timer timer_;
  RetryTimer retry_timer_;
  std::string url_;

  void BuildUrl();

  bool IsSupportedLocale(
      const std::string& locale) const;

  void MaybeAllowAdsSubdivisionTargetingForLocale(
      const std::string& locale);

  bool ShouldAutomaticallyDetect() const;

  void Fetch();

  void OnFetch(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  bool ParseJson(
      const std::string& json);

  void Retry();

  void FetchAfterDelay();

  AdsClient* ads_client_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SUBDIVISION_TARGETING_H_
