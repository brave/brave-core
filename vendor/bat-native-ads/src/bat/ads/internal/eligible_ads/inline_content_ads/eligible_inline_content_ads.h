/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace inline_content_ads {

using GetEligibleAdsCallback =
    std::function<void(const bool, const CreativeInlineContentAdList&)>;

class EligibleAds {
 public:
  EligibleAds(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting);

  ~EligibleAds();

  void SetLastServedAd(const CreativeAdInfo& creative_ad);

  void GetForSegments(const SegmentList& segments,
                      const std::string& dimensions,
                      GetEligibleAdsCallback callback);

 private:
  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED

  CreativeAdInfo last_served_creative_ad_;

  void GetForParentChildSegments(const SegmentList& segments,
                                 const std::string& dimensions,
                                 const AdEventList& ad_events,
                                 const BrowsingHistoryList& browsing_history,
                                 GetEligibleAdsCallback callback) const;

  void GetForParentSegments(const SegmentList& segments,
                            const std::string& dimensions,
                            const AdEventList& ad_events,
                            const BrowsingHistoryList& browsing_history,
                            GetEligibleAdsCallback callback) const;

  void GetForUntargeted(const std::string& dimensions,
                        const AdEventList& ad_events,
                        const BrowsingHistoryList& browsing_history,
                        GetEligibleAdsCallback callback) const;

  CreativeInlineContentAdList FilterIneligibleAds(
      const CreativeInlineContentAdList& ads,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history) const;

  CreativeInlineContentAdList ApplyFrequencyCapping(
      const CreativeInlineContentAdList& ads,
      const CreativeAdInfo& last_served_creative_ad,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history) const;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_H_
