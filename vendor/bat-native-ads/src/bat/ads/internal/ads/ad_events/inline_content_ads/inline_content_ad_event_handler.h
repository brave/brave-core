/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct InlineContentAdInfo;

namespace inline_content_ads {

class EventHandler final : public EventHandlerObserver {
 public:
  EventHandler();
  ~EventHandler() override;
  EventHandler(const EventHandler&) = delete;
  EventHandler& operator=(const EventHandler&) = delete;

  void AddObserver(EventHandlerObserver* observer);
  void RemoveObserver(EventHandlerObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::InlineContentAdEventType event_type);

 private:
  void FireEvent(const InlineContentAdInfo& ad,
                 const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::InlineContentAdEventType event_type);
  void FailedToFireEvent(const std::string& placement_id,
                         const std::string& creative_instance_id,
                         mojom::InlineContentAdEventType event_type) const;

  void NotifyInlineContentAdEvent(
      const InlineContentAdInfo& ad,
      mojom::InlineContentAdEventType event_type) const;
  void NotifyInlineContentAdServed(const InlineContentAdInfo& ad) const;
  void NotifyInlineContentAdViewed(const InlineContentAdInfo& ad) const;
  void NotifyInlineContentAdClicked(const InlineContentAdInfo& ad) const;
  void NotifyInlineContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType event_type) const;

  base::ObserverList<EventHandlerObserver> observers_;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
