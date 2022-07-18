/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_

#include <string>

#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_callback.h"
#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/exclusion_rule_alias.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"

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

namespace inline_content_ads {

class EligibleAdsV1 final : public EligibleAdsBase {
 public:
  EligibleAdsV1(geographic::SubdivisionTargeting* subdivision_targeting,
                resource::AntiTargeting* anti_targeting);
  ~EligibleAdsV1() override;
  EligibleAdsV1(const EligibleAdsV1&) = delete;
  EligibleAdsV1& operator=(const EligibleAdsV1&) = delete;

  void GetForUserModel(
      const targeting::UserModelInfo& user_model,
      const std::string& dimensions,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback) override;

 private:
  void GetEligibleAds(
      const targeting::UserModelInfo& user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback);

  void GetForChildSegments(
      const targeting::UserModelInfo& user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback);

  void GetForParentSegments(
      const targeting::UserModelInfo& user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback);

  void GetForUntargeted(
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback);

  CreativeInlineContentAdList FilterCreativeAds(
      const CreativeInlineContentAdList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_
