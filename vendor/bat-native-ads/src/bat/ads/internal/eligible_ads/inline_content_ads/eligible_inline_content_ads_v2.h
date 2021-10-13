/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info_aliases.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info_aliases.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_aliases.h"
#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_base.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_aliases.h"

namespace ads {

namespace ad_targeting {
struct UserModelInfo;
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

struct AdInfo;

namespace inline_content_ads {

class EligibleAdsV2 final : public EligibleAdsBase {
 public:
  EligibleAdsV2(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting);
  ~EligibleAdsV2() override;

  void GetForUserModel(const ad_targeting::UserModelInfo& user_model,
                       const std::string& dimensions,
                       GetEligibleAdsCallback callback) override;

 private:
  void GetEligibleAds(const ad_targeting::UserModelInfo& user_model,
                      const AdEventList& ad_events,
                      const BrowsingHistoryList& browsing_history,
                      const std::string& dimensions,
                      GetEligibleAdsCallback callback) const;

  CreativeInlineContentAdList ApplyFrequencyCapping(
      const CreativeInlineContentAdList& creative_ads,
      const AdInfo& last_served_ad,
      const AdEventList& ad_events,
      const BrowsingHistoryList& browsing_history) const;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_V2_H_
