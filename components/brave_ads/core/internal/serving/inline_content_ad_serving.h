/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_INLINE_CONTENT_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_INLINE_CONTENT_AD_SERVING_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class EligibleInlineContentAdsBase;
class SubdivisionTargeting;
struct InlineContentAdInfo;

class InlineContentAdServing final {
 public:
  InlineContentAdServing(const SubdivisionTargeting& subdivision_targeting,
                         const AntiTargetingResource& anti_targeting_resource);

  InlineContentAdServing(const InlineContentAdServing&) = delete;
  InlineContentAdServing& operator=(const InlineContentAdServing&) = delete;

  ~InlineContentAdServing();

  void SetDelegate(InlineContentAdServingDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback);

 private:
  bool IsSupported() const { return !!eligible_ads_; }

  bool CanServeAd(const AdEventList& ad_events) const;

  void GetAdEvents(int32_t tab_id,
                   const std::string& dimensions,
                   MaybeServeInlineContentAdCallback callback);
  void GetAdEventsCallback(int32_t tab_id,
                           const std::string& dimensions,
                           MaybeServeInlineContentAdCallback callback,
                           bool success,
                           const AdEventList& ad_events);

  void GetUserModel(int32_t tab_id,
                    const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback);
  void GetUserModelCallback(int32_t tab_id,
                            const std::string& dimensions,
                            MaybeServeInlineContentAdCallback callback,
                            UserModelInfo user_model) const;

  void GetEligibleAds(int32_t tab_id,
                      const std::string& dimensions,
                      MaybeServeInlineContentAdCallback callback,
                      UserModelInfo user_model) const;
  void GetEligibleAdsCallback(
      int32_t tab_id,
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback,
      const CreativeInlineContentAdList& creative_ads) const;

  void ServeAd(int32_t tab_id,
               const InlineContentAdInfo& ad,
               MaybeServeInlineContentAdCallback callback) const;
  void SuccessfullyServedAd(int32_t tab_id,
                            const InlineContentAdInfo& ad,
                            MaybeServeInlineContentAdCallback callback) const;
  void FailedToServeAd(const std::string& dimensions,
                       MaybeServeInlineContentAdCallback callback) const;

  void NotifyOpportunityAroseToServeInlineContentAd() const;
  void NotifyDidServeInlineContentAd(int32_t tab_id,
                                     const InlineContentAdInfo& ad) const;
  void NotifyFailedToServeInlineContentAd() const;

  raw_ptr<InlineContentAdServingDelegate> delegate_ = nullptr;  // Not owned.

  std::unique_ptr<EligibleInlineContentAdsBase> eligible_ads_;

  base::WeakPtrFactory<InlineContentAdServing> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_INLINE_CONTENT_AD_SERVING_H_
