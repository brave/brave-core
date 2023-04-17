/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V1_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V1_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_callback.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

class SubdivisionTargeting;

namespace new_tab_page_ads {

class EligibleAdsV1 final : public EligibleAdsBase {
 public:
  EligibleAdsV1(const SubdivisionTargeting& subdivision_targeting,
                const resource::AntiTargeting& anti_targeting_resource);
  ~EligibleAdsV1() override;

  void GetForUserModel(
      targeting::UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback) override;

 private:
  void OnGetForUserModel(
      targeting::UserModelInfo user_model,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const AdEventList& ad_events);

  void GetBrowsingHistory(
      targeting::UserModelInfo user_model,
      const AdEventList& ad_events,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback);

  void GetEligibleAds(targeting::UserModelInfo user_model,
                      const AdEventList& ad_events,
                      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
                      const BrowsingHistoryList& browsing_history);

  void GetForChildSegments(
      targeting::UserModelInfo user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback);

  void OnGetForChildSegments(
      const targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeNewTabPageAdList& creative_ads);

  void GetForParentSegments(
      const targeting::UserModelInfo& user_model,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback);

  void OnGetForParentSegments(
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeNewTabPageAdList& creative_ads);

  void GetForUntargeted(
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback);

  void OnGetForUntargeted(
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeNewTabPageAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeNewTabPageAdList& creative_ads);

  CreativeNewTabPageAdList FilterCreativeAds(
      const CreativeNewTabPageAdList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);

  base::WeakPtrFactory<EligibleAdsV1> weak_factory_{this};
};

}  // namespace new_tab_page_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V1_H_
