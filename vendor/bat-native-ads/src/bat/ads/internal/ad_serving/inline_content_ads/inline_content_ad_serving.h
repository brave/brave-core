/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_SERVING_H_

#include <memory>
#include <string>

#include "bat/ads/ads.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_serving/inline_content_ads/inline_content_ad_serving_observer.h"

namespace ads {

class AdTargeting;

namespace ad_targeting {
namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic
}  // namespace ad_targeting

namespace resource {
class AntiTargeting;
}  // namespace resource

namespace inline_content_ads {

class EligibleAds;

class AdServing {
 public:
  AdServing(
      AdTargeting* ad_targeting,
      ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
      resource::AntiTargeting* anti_targeting_resource);

  ~AdServing();

  void AddObserver(InlineContentAdServingObserver* observer);
  void RemoveObserver(InlineContentAdServingObserver* observer);

  void MaybeServeAd(const std::string& dimensions,
                    GetInlineContentAdCallback callback);

 private:
  AdTargeting* ad_targeting_;  // NOT OWNED

  ad_targeting::geographic::SubdivisionTargeting*
      subdivision_targeting_;  // NOT OWNED

  resource::AntiTargeting* anti_targeting_resource_;  // NOT OWNED

  std::unique_ptr<EligibleAds> eligible_ads_;

  base::ObserverList<InlineContentAdServingObserver> observers_;

  void NotifyDidServeInlineContentAd(const InlineContentAdInfo& ad) const;
  void NotifyFailedToServeInlineContentAd() const;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_SERVING_H_
