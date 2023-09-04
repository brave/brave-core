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
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class EligibleNewTabPageAdsBase;
class SubdivisionTargeting;
struct NewTabPageAdInfo;
struct UserModelInfo;

class NewTabPageAdServing final {
 public:
  NewTabPageAdServing(const SubdivisionTargeting& subdivision_targeting,
                      const AntiTargetingResource& anti_targeting_resource);

  NewTabPageAdServing(const NewTabPageAdServing&) = delete;
  NewTabPageAdServing& operator=(const NewTabPageAdServing&) = delete;

  NewTabPageAdServing(NewTabPageAdServing&&) noexcept = delete;
  NewTabPageAdServing& operator=(NewTabPageAdServing&&) noexcept = delete;

  ~NewTabPageAdServing();

  void SetDelegate(NewTabPageAdServingDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeServeAd(MaybeServeNewTabPageAdCallback callback);

 private:
  bool IsSupported() const { return bool{eligible_ads_}; }

  void BuildUserModelCallback(MaybeServeNewTabPageAdCallback callback,
                              const UserModelInfo& user_model);
  void GetForUserModelCallback(MaybeServeNewTabPageAdCallback callback,
                               const UserModelInfo& user_model,
                               bool had_opportunity,
                               const CreativeNewTabPageAdList& creative_ads);

  void ServeAd(const NewTabPageAdInfo& ad,
               MaybeServeNewTabPageAdCallback callback);
  void FailedToServeAd(MaybeServeNewTabPageAdCallback callback);

  raw_ptr<NewTabPageAdServingDelegate> delegate_ = nullptr;

  std::unique_ptr<EligibleNewTabPageAdsBase> eligible_ads_;

  base::WeakPtrFactory<NewTabPageAdServing> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
