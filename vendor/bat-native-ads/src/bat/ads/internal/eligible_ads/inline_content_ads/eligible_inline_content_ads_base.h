/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_

#include "bat/ads/internal/eligible_ads/inline_content_ads/eligible_inline_content_ads_aliases.h"

#include <string>

#include "bat/ads/ad_info.h"

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

namespace inline_content_ads {

class EligibleAdsBase {
 public:
  EligibleAdsBase(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);
  virtual ~EligibleAdsBase();

  virtual void GetForUserModel(const ad_targeting::UserModelInfo& user_model,
                               const std::string& dimensions,
                               GetEligibleAdsCallback callback) = 0;

  void set_last_served_ad(const AdInfo& ad) { last_served_ad_ = ad; }

 protected:
  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED

  AdInfo last_served_ad_;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
