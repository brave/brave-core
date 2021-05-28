/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_observer.h"
#include "bat/ads/mojom.h"

namespace ads {

struct InlineContentAdInfo;

class InlineContentAd : public InlineContentAdObserver {
 public:
  InlineContentAd();

  ~InlineContentAd() override;

  void AddObserver(InlineContentAdObserver* observer);
  void RemoveObserver(InlineContentAdObserver* observer);

  void FireEvent(const std::string& uuid,
                 const std::string& creative_instance_id,
                 const InlineContentAdEventType event_type);

 private:
  base::ObserverList<InlineContentAdObserver> observers_;

  void FireEvent(const InlineContentAdInfo& ad,
                 const std::string& uuid,
                 const std::string& creative_instance_id,
                 const InlineContentAdEventType event_type);

  void NotifyInlineContentAdEvent(
      const InlineContentAdInfo& ad,
      const InlineContentAdEventType event_type) const;

  void NotifyInlineContentAdServed(const InlineContentAdInfo& ad) const;
  void NotifyInlineContentAdViewed(const InlineContentAdInfo& ad) const;
  void NotifyInlineContentAdClicked(const InlineContentAdInfo& ad) const;

  void NotifyInlineContentAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const InlineContentAdEventType event_type) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_H_
