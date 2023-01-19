/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_

#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_callback.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/new_tab_page_ads/eligible_new_tab_page_ads_base.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

namespace new_tab_page_ads {

class EligibleAdsV2 final : public EligibleAdsBase {
 public:
  EligibleAdsV2(geographic::SubdivisionTargeting* subdivision_targeting,
                resource::AntiTargeting* anti_targeting);

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

  void OnGetEligibleAds(
      const targeting::UserModelInfo& user_model,
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
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_NEW_TAB_PAGE_ADS_ELIGIBLE_NEW_TAB_PAGE_ADS_V2_H_
