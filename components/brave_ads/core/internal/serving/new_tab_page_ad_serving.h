/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_H_

#include <cstdint>
#include <memory>
#include <optional>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class EligibleNewTabPageAdsBase;
class SubdivisionTargeting;
struct NewTabPageAdInfo;

class NewTabPageAdServing final {
 public:
  NewTabPageAdServing(const SubdivisionTargeting& subdivision_targeting,
                      const AntiTargetingResource& anti_targeting_resource);

  NewTabPageAdServing(const NewTabPageAdServing&) = delete;
  NewTabPageAdServing& operator=(const NewTabPageAdServing&) = delete;

  ~NewTabPageAdServing();

  void SetDelegate(NewTabPageAdServingDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeServeAd(MaybeServeNewTabPageAdCallback callback);

 private:
  bool IsSupported() const { return !!eligible_ads_; }

  bool CanServeAd(const AdEventList& ad_events) const;

  void GetAdEvents();
  void GetAdEventsCallback(bool success, const AdEventList& ad_events);

  void GetUserModel();
  void GetUserModelCallback(uint64_t trace_id, UserModelInfo user_model);

  void GetEligibleAds(UserModelInfo user_model);
  void GetEligibleAdsCallback(uint64_t trace_id,
                              CreativeNewTabPageAdList creative_ads);

  void ServeAd(const NewTabPageAdInfo& ad);
  void SuccessfullyServedAd(const NewTabPageAdInfo& ad);
  void FailedToServeAd();

  void NotifyOpportunityAroseToServeNewTabPageAd() const;
  void NotifyDidServeNewTabPageAd(const NewTabPageAdInfo& ad) const;
  void NotifyFailedToServeNewTabPageAd() const;

  raw_ptr<NewTabPageAdServingDelegate> delegate_ = nullptr;  // Not owned.

  std::unique_ptr<EligibleNewTabPageAdsBase> eligible_ads_;

  // Holds the callback for the in-flight serve request. A new call to
  // `MaybeServeAd` while a pipeline is running cancels the old pipeline and
  // fails the superseded callback before starting a fresh one.
  std::optional<MaybeServeNewTabPageAdCallback> pending_serve_ad_callback_;

  // Invalidated on each new `MaybeServeAd` call to cancel any in-flight
  // pipeline before starting a fresh one.
  base::WeakPtrFactory<NewTabPageAdServing> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
