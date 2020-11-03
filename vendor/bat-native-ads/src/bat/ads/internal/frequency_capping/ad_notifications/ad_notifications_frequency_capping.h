/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_NOTIFICATIONS_AD_NOTIFICATIONS_FREQUENCY_CAPPING_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_NOTIFICATIONS_AD_NOTIFICATIONS_FREQUENCY_CAPPING_H_  // NOLINT

#include "bat/ads/internal/ad_events/ad_event_info.h"

namespace ads {

class AdsImpl;
struct CreativeAdInfo;

namespace ad_notifications {

class FrequencyCapping {
 public:
  FrequencyCapping(
      AdsImpl* ads,
      const AdEventList& ad_events);

  ~FrequencyCapping();

  FrequencyCapping(
      const FrequencyCapping&) = delete;
  FrequencyCapping& operator=(
      const FrequencyCapping&) = delete;

  bool IsAdAllowed();

  bool ShouldExcludeAd(
      const CreativeAdInfo& ad);

 private:
  AdsImpl* ads_;  // NOT OWNED

  AdEventList ad_events_;
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_AD_NOTIFICATIONS_AD_NOTIFICATIONS_FREQUENCY_CAPPING_H_  // NOLINT
