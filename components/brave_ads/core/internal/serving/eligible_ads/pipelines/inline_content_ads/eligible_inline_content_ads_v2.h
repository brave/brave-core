/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_

#include <cstdint>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_callback.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads {

class AntiTargetingResource;
class SubdivisionTargeting;
struct UserModelInfo;

class EligibleInlineContentAdsV2 final : public EligibleInlineContentAdsBase {
 public:
  EligibleInlineContentAdsV2(
      const SubdivisionTargeting& subdivision_targeting,
      const AntiTargetingResource& anti_targeting_resource);
  ~EligibleInlineContentAdsV2() override;

  void GetForUserModel(
      UserModelInfo user_model,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback) override;

 private:
  void GetForUserModelCallback(
      UserModelInfo user_model,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetSiteHistory(
      UserModelInfo user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void GetSiteHistoryCallback(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      uint64_t trace_id,
      const SiteHistoryList& site_history);

  void GetEligibleAds(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void GetEligibleAdsCallback(
      const UserModelInfo& user_model,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const CreativeInlineContentAdList& creative_ads);

  void FilterAndMaybePredictCreativeAd(
      const UserModelInfo& user_model,
      const CreativeInlineContentAdList& creative_ads,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void FilterIneligibleCreativeAds(CreativeInlineContentAdList& creative_ads,
                                   const AdEventList& ad_events,
                                   const SiteHistoryList& site_history);

  const database::table::CreativeInlineContentAds creative_ads_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  base::WeakPtrFactory<EligibleInlineContentAdsV2> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_
