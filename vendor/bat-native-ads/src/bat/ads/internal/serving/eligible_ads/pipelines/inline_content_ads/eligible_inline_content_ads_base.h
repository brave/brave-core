/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info_aliases.h"
#include "bat/ads/internal/serving/eligible_ads/eligible_ads_callback.h"

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

class EligibleAdsBase {
 public:
  virtual ~EligibleAdsBase();

  virtual void GetForUserModel(
      const targeting::UserModelInfo& user_model,
      const std::string& dimensions,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback) = 0;

  void SetLastServedAd(const AdInfo& ad) { last_served_ad_ = ad; }

 protected:
  EligibleAdsBase(geographic::SubdivisionTargeting* subdivision_targeting,
                  resource::AntiTargeting* anti_targeting_resource);

  const raw_ptr<geographic::SubdivisionTargeting> subdivision_targeting_ =
      nullptr;  // NOT OWNED

  const raw_ptr<resource::AntiTargeting> anti_targeting_resource_ =
      nullptr;  // NOT OWNED

  AdInfo last_served_ad_;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
