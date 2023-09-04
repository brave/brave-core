/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class EligibleInlineContentAdsBase;
class SubdivisionTargeting;
struct InlineContentAdInfo;
struct UserModelInfo;

class InlineContentAdServing final {
 public:
  InlineContentAdServing(const SubdivisionTargeting& subdivision_targeting,
                         const AntiTargetingResource& anti_targeting_resource);

  InlineContentAdServing(const InlineContentAdServing&) = delete;
  InlineContentAdServing& operator=(const InlineContentAdServing&) = delete;

  InlineContentAdServing(InlineContentAdServing&&) noexcept = delete;
  InlineContentAdServing& operator=(InlineContentAdServing&&) noexcept = delete;

  ~InlineContentAdServing();

  void SetDelegate(InlineContentAdServingDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback);

 private:
  bool IsSupported() const { return bool{eligible_ads_}; }

  void BuildUserModelCallback(const std::string& dimensions,
                              MaybeServeInlineContentAdCallback callback,
                              const UserModelInfo& user_model);
  void GetForUserModelCallback(const UserModelInfo& user_model,
                               const std::string& dimensions,
                               MaybeServeInlineContentAdCallback callback,
                               bool had_opportunity,
                               const CreativeInlineContentAdList& creative_ads);

  void ServeAd(const InlineContentAdInfo& ad,
               MaybeServeInlineContentAdCallback callback);
  void FailedToServeAd(const std::string& dimensions,
                       MaybeServeInlineContentAdCallback callback);

  raw_ptr<InlineContentAdServingDelegate> delegate_ = nullptr;

  std::unique_ptr<EligibleInlineContentAdsBase> eligible_ads_;

  base::WeakPtrFactory<InlineContentAdServing> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_
