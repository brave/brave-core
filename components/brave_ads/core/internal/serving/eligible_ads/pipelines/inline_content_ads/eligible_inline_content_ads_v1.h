/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/history/browsing_history.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_callback.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/inline_content_ads/eligible_inline_content_ads_base.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"

namespace brave_ads {

class AntiTargetingResource;
class SubdivisionTargeting;
struct UserModelInfo;

class EligibleInlineContentAdsV1 final : public EligibleInlineContentAdsBase {
 public:
  EligibleInlineContentAdsV1(
      const SubdivisionTargeting& subdivision_targeting,
      const AntiTargetingResource& anti_targeting_resource);
  ~EligibleInlineContentAdsV1() override;

  void GetForUserModel(
      UserModelInfo user_model,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback) override;

 private:
  void GetEligibleAdsForUserModelCallback(
      UserModelInfo user_model,
      const std::string& dimensions,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const AdEventList& ad_events);
  void GetEligibleAds(UserModelInfo user_model,
                      const std::string& dimensions,
                      const AdEventList& ad_events,
                      EligibleAdsCallback<CreativeInlineContentAdList> callback,
                      const BrowsingHistoryList& browsing_history);

  void GetForChildSegments(
      UserModelInfo user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void GetForChildSegmentsCallback(
      const UserModelInfo& user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeInlineContentAdList& creative_ads);

  void GetForParentSegments(
      const UserModelInfo& user_model,
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void GetForParentSegmentsCallback(
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeInlineContentAdList& creative_ads);

  void GetForUntargeted(
      const std::string& dimensions,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback);
  void GetForUntargetedCallback(
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history,
      EligibleAdsCallback<CreativeInlineContentAdList> callback,
      bool success,
      const SegmentList& segments,
      const CreativeInlineContentAdList& creative_ads);

  CreativeInlineContentAdList FilterCreativeAds(
      const CreativeInlineContentAdList& creative_ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history);

  base::WeakPtrFactory<EligibleInlineContentAdsV1> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V1_H_
