/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_callback.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"

namespace brave_ads {

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
  EligibleAdsBase(const EligibleAdsBase&) = delete;
  EligibleAdsBase& operator=(const EligibleAdsBase&) = delete;

  EligibleAdsBase(EligibleAdsBase&&) noexcept = delete;
  EligibleAdsBase& operator=(EligibleAdsBase&&) noexcept = delete;

  virtual ~EligibleAdsBase();

  virtual void GetForUserModel(
      targeting::UserModelInfo user_model,
      const std::string& dimensions,
      GetEligibleAdsCallback<CreativeInlineContentAdList> callback) = 0;

  void SetLastServedAd(const AdInfo& ad) { last_served_ad_ = ad; }

 protected:
  EligibleAdsBase(const geographic::SubdivisionTargeting& subdivision_targeting,
                  const resource::AntiTargeting& anti_targeting_resource);

  const base::raw_ref<const geographic::SubdivisionTargeting>
      subdivision_targeting_;
  const base::raw_ref<const resource::AntiTargeting> anti_targeting_resource_;

  AdInfo last_served_ad_;
};

}  // namespace inline_content_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_ELIGIBLE_ADS_PIPELINES_INLINE_CONTENT_ADS_ELIGIBLE_INLINE_CONTENT_ADS_BASE_H_
