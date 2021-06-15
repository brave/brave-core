/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

struct CreativeAdInfo;

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace ad_notifications {
namespace frequency_capping {

class ExclusionRules {
 public:
  ExclusionRules(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);

  ~ExclusionRules();

  bool ShouldExcludeAd(const CreativeAdInfo& ad) const;

 private:
  ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting_;
  resource::AntiTargeting* anti_targeting_resource_;
  AdEventList ad_events_;
  BrowsingHistoryList browsing_history_;

  ExclusionRules(const ExclusionRules&) = delete;
  ExclusionRules& operator=(const ExclusionRules&) = delete;
};

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_NOTIFICATIONS_AD_NOTIFICATION_EXCLUSION_RULES_H_
