/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_

#include <cstdint>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

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
      EligibleAdsCallback<CreativeNotificationAdList> callback) override;

 private:
  void GetForUserModelCallback(
      UserModelInfo user_model,
      EligibleAdsCallback<CreativeNotificationAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetSiteHistory(UserModelInfo user_model,
                      const AdEventList& ad_events,
                      EligibleAdsCallback<CreativeNotificationAdList> callback);
  void GetSiteHistoryCallback(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      EligibleAdsCallback<CreativeNotificationAdList> callback,
      uint64_t trace_id,
      const SiteHistoryList& site_history);

  void GetEligibleAds(UserModelInfo user_model,
                      const AdEventList& ad_events,
                      const SiteHistoryList& site_history,
                      EligibleAdsCallback<CreativeNotificationAdList> callback);
  void GetEligibleAdsCallback(
      const UserModelInfo& user_model,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeNotificationAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeNotificationAdList& creative_ads);

  void FilterAndMaybePredictCreativeAd(
      const UserModelInfo& user_model,
      const CreativeNotificationAdList& creative_ads,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeNotificationAdList> callback);
  void FilterIneligibleCreativeAds(CreativeNotificationAdList& creative_ads,
                                   const AdEventList& ad_events,
                                   const SiteHistoryList& site_history);

  const database::table::CreativeNotificationAds creative_ads_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  base::WeakPtrFactory<EligibleNotificationAdsV2> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NOTIFICATION_ADS_ELIGIBLE_NOTIFICATION_ADS_V2_H_
