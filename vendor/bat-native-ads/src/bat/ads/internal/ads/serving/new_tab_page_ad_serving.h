/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_

#include <memory>

#include "base/observer_list.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving_observer.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

struct NewTabPageAdInfo;

namespace new_tab_page_ads {

class EligibleAdsBase;

class Serving final {
 public:
  Serving(geographic::SubdivisionTargeting* subdivision_targeting,
          resource::AntiTargeting* anti_targeting_resource);

  Serving(const Serving& other) = delete;
  Serving& operator=(const Serving& other) = delete;

  Serving(Serving&& other) noexcept = delete;
  Serving& operator=(Serving&& other) noexcept = delete;

  ~Serving();

  void AddObserver(ServingObserver* observer);
  void RemoveObserver(ServingObserver* observer);

  void MaybeServeAd(const MaybeServeNewTabPageAdCallback& callback);

 private:
  bool IsSupported() const;

  void ServeAd(const NewTabPageAdInfo& ad,
               const MaybeServeNewTabPageAdCallback& callback);
  void FailedToServeAd(const MaybeServeNewTabPageAdCallback& callback);

  void NotifyOpportunityAroseToServeNewTabPageAd(
      const SegmentList& segments) const;
  void NotifyDidServeNewTabPageAd(const NewTabPageAdInfo& ad) const;
  void NotifyFailedToServeNewTabPageAd() const;

  base::ObserverList<ServingObserver> observers_;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_NEW_TAB_PAGE_AD_SERVING_H_
