/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad_observer.h"
#include "bat/ads/mojom.h"

namespace ads {

struct PromotedContentAdInfo;

class PromotedContentAd : public PromotedContentAdObserver {
 public:
  PromotedContentAd();

  ~PromotedContentAd() override;

  void AddObserver(PromotedContentAdObserver* observer);
  void RemoveObserver(PromotedContentAdObserver* observer);

  void FireEvent(const std::string& uuid,
                 const std::string& creative_instance_id,
                 const PromotedContentAdEventType event_type);

 private:
  base::ObserverList<PromotedContentAdObserver> observers_;

  void FireEvent(const PromotedContentAdInfo& ad,
                 const std::string& uuid,
                 const std::string& creative_instance_id,
                 const PromotedContentAdEventType event_type);

  void NotifyPromotedContentAdEvent(
      const PromotedContentAdInfo& ad,
      const PromotedContentAdEventType event_type) const;

  void NotifyPromotedContentAdServed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdViewed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdClicked(const PromotedContentAdInfo& ad) const;

  void NotifyPromotedContentAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const PromotedContentAdEventType event_type) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_H_
