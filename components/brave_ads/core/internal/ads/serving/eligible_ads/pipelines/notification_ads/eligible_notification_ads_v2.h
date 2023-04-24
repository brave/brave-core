/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

class AntiTargetingResource;
class SubdivisionTargeting;
struct UserModelInfo;

class EligibleNotificationAdsV2 final : public EligibleNotificationAdsBase {
 public:
  EligibleNotificationAdsV2(
      const SubdivisionTargeting& subdivision_targeting,
      const AntiTargetingResource& anti_targeting_resource);
  ~EligibleNotificationAdsV2() override;

  void GetForUserModel(
      UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback) override;

 private:
  void OnGetForUserModel(
      UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetBrowsingHistory(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback);

  void GetEligibleAds(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNotificationAdList> callback,
      const BrowsingHistoryList& browsing_history);
  void OnGetEligibleAds(
      const UserModelInfo& user_model,
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

  base::WeakPtrFactory<EligibleNotificationAdsV2> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_
