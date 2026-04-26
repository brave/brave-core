/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_

#include <cstdint>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/segments/segment_types.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_callback.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads {

class AntiTargetingResource;
class CreativeAdRoundRobin;
class SubdivisionTargeting;
struct UserModelInfo;

class EligibleNewTabPageAdsV2 final : public EligibleNewTabPageAdsBase {
 public:
  EligibleNewTabPageAdsV2(const SubdivisionTargeting& subdivision_targeting,
                          const AntiTargetingResource& anti_targeting_resource,
                          CreativeAdRoundRobin& creative_ad_round_robin);

  ~EligibleNewTabPageAdsV2() override;

  // EligibleNewTabPageAdsBase:
  void GetForUserModel(
      UserModelInfo user_model,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback) override;

 private:
  void GetForUserModelCallback(
      UserModelInfo user_model,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetSiteHistory(UserModelInfo user_model,
                      const AdEventList& ad_events,
                      EligibleAdsCallback<CreativeNewTabPageAdList> callback);
  void GetSiteHistoryCallback(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback,
      uint64_t trace_id,
      const SiteHistoryList& site_history);

  void GetEligibleAds(UserModelInfo user_model,
                      const AdEventList& ad_events,
                      const SiteHistoryList& site_history,
                      EligibleAdsCallback<CreativeNewTabPageAdList> callback);
  void GetEligibleAdsCallback(
      UserModelInfo user_model,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const SegmentList& segments,
      CreativeNewTabPageAdList creative_ads);

  void ApplyConditionMatcher(
      CreativeNewTabPageAdList creative_ads,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback);
  void ApplyConditionMatcherCallback(
      CreativeNewTabPageAdList creative_ads,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback,
      base::DictValue virtual_prefs);

  void FilterAndMaybePredictCreativeAd(
      UserModelInfo user_model,
      CreativeNewTabPageAdList creative_ads,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback);
  void FilterAndMaybePredictCreativeAdCallback(
      const UserModelInfo& user_model,
      const AdEventList& ad_events,
      const SiteHistoryList& site_history,
      EligibleAdsCallback<CreativeNewTabPageAdList> callback,
      uint64_t trace_id,
      CreativeNewTabPageAdList creative_ads);

  const database::table::CreativeNewTabPageAds creative_ads_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  // Must outlive `this`. Guaranteed by `NewTabPageAdServing`, which owns both
  // and declares `creative_ad_round_robin_` before `eligible_ads_`.
  const raw_ref<CreativeAdRoundRobin> creative_ad_round_robin_;

  base::WeakPtrFactory<EligibleNewTabPageAdsV2> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_
