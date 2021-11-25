/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/internal/ad_serving/new_tab_page_ads/new_tab_page_ad_serving_observer.h"

namespace ads {

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

struct NewTabPageAdInfo;

namespace new_tab_page_ads {

class EligibleAdsBase;

class AdServing final {
 public:
  AdServing(
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);
  ~AdServing();

  void AddObserver(NewTabPageAdServingObserver* observer);
  void RemoveObserver(NewTabPageAdServingObserver* observer);

  void MaybeServeAd(GetNewTabPageAdCallback callback);

 private:
  std::unique_ptr<EligibleAdsBase> eligible_ads_;
  bool IsSupported() const;

  bool ServeAd(const NewTabPageAdInfo& ad,
               GetNewTabPageAdCallback callback) const;
  void FailedToServeAd(GetNewTabPageAdCallback callback);
  void ServedAd(const NewTabPageAdInfo& ad);

  base::ObserverList<NewTabPageAdServingObserver> observers_;

  void NotifyDidServeNewTabPageAd(const NewTabPageAdInfo& ad) const;
  void NotifyFailedToServeNewTabPageAd() const;
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_H_
