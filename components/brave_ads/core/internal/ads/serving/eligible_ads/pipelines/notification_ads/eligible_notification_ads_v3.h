/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V3_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V3_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

class SubdivisionTargeting;

namespace notification_ads {

class EligibleAdsV3 final : public EligibleAdsBase {
 public:
  EligibleAdsV3(const SubdivisionTargeting& subdivision_targeting,
                const resource::AntiTargeting& anti_targeting_resource);
  ~EligibleAdsV3() override;

  void GetForUserModel(
      targeting::UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback) override;

 private:
  void OnGetForUserModel(
      targeting::UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetBrowsingHistory(
      targeting::UserModelInfo user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback);

  void GetEligibleAds(
      targeting::UserModelInfo user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      const BrowsingHistoryList& browsing_history);

  void OnGetEligibleAds(
      const targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeNotificationAdList& creative_ads);

  CreativeNotificationAdList FilterCreativeAds(
      const CreativeNotificationAdList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);

  base::WeakPtrFactory<EligibleAdsV3> weak_factory_{this};
};

}  // namespace notification_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V3_H_
