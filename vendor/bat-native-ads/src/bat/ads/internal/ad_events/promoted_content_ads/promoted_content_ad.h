/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct PromotedContentAdInfo;

class PromotedContentAd final : public PromotedContentAdObserver {
 public:
  PromotedContentAd();
  ~PromotedContentAd() override;
  PromotedContentAd(const PromotedContentAd&) = delete;
  PromotedContentAd& operator=(const PromotedContentAd&) = delete;

  void AddObserver(PromotedContentAdObserver* observer);
  void RemoveObserver(PromotedContentAdObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::PromotedContentAdEventType event_type);

 private:
  void FireEvent(const PromotedContentAdInfo& ad,
                 const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::PromotedContentAdEventType event_type);

  void NotifyPromotedContentAdEvent(
      const PromotedContentAdInfo& ad,
      const mojom::PromotedContentAdEventType event_type) const;
  void NotifyPromotedContentAdServed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdViewed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdClicked(const PromotedContentAdInfo& ad) const;

  void NotifyPromotedContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type) const;

  base::ObserverList<PromotedContentAdObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_
