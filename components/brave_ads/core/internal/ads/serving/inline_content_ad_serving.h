/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_observer.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

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

struct InlineContentAdInfo;

namespace inline_content_ads {

class EligibleAdsBase;

class Serving final {
 public:
  Serving(const geographic::SubdivisionTargeting& subdivision_targeting,
          const resource::AntiTargeting& anti_targeting_resource);

  Serving(const Serving&) = delete;
  Serving& operator=(const Serving&) = delete;

  Serving(Serving&&) noexcept = delete;
  Serving& operator=(Serving&&) noexcept = delete;

  ~Serving();

  void AddObserver(ServingObserver* observer);
  void RemoveObserver(ServingObserver* observer);

  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback);

 private:
  void OnBuildUserModel(const std::string& dimensions,
                        MaybeServeInlineContentAdCallback callback,
                        const targeting::UserModelInfo& user_model);
  void OnGetForUserModel(const targeting::UserModelInfo& user_model,
                         const std::string& dimensions,
                         MaybeServeInlineContentAdCallback callback,
                         bool had_opportunity,
                         const CreativeInlineContentAdList& creative_ads);

  bool IsSupported() const { return static_cast<bool>(eligible_ads_); }

  void ServeAd(const InlineContentAdInfo& ad,
               MaybeServeInlineContentAdCallback callback);
  void FailedToServeAd(const std::string& dimensions,
                       MaybeServeInlineContentAdCallback callback);

  void NotifyOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) const;
  void NotifyDidServeInlineContentAd(const InlineContentAdInfo& ad) const;
  void NotifyFailedToServeInlineContentAd() const;

  base::ObserverList<ServingObserver> observers_;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;

  base::WeakPtrFactory<Serving> weak_factory_{this};
};

}  // namespace inline_content_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_
