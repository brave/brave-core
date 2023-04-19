/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_

#include <memory>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"

namespace brave_ads {

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace targeting {
struct UserModelInfo;
}  // namespace targeting

class SubdivisionTargeting;
struct NewTabPageAdInfo;

namespace new_tab_page_ads {

class EligibleAdsBase;

class Serving final {
 public:
  Serving(const SubdivisionTargeting& subdivision_targeting,
          const resource::AntiTargeting& anti_targeting_resource);

  Serving(const Serving&) = delete;
  Serving& operator=(const Serving&) = delete;

  Serving(Serving&&) noexcept = delete;
  Serving& operator=(Serving&&) noexcept = delete;

  ~Serving();

  void SetDelegate(ServingDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeServeAd(MaybeServeNewTabPageAdCallback callback);

 private:
  bool IsSupported() const { return static_cast<bool>(eligible_ads_); }

  void OnBuildUserModel(MaybeServeNewTabPageAdCallback callback,
                        const targeting::UserModelInfo& user_model);
  void OnGetForUserModel(MaybeServeNewTabPageAdCallback callback,
                         const targeting::UserModelInfo& user_model,
                         bool had_opportunity,
                         const CreativeNewTabPageAdList& creative_ads);

  void ServeAd(const NewTabPageAdInfo& ad,
               MaybeServeNewTabPageAdCallback callback);
  void FailedToServeAd(MaybeServeNewTabPageAdCallback callback);

  raw_ptr<ServingDelegate> delegate_ = nullptr;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;

  base::WeakPtrFactory<Serving> weak_factory_{this};
};

}  // namespace new_tab_page_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
