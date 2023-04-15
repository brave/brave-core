/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_observer.h"

namespace brave_ads {

struct CreativeInlineContentAdInfo;
struct InlineContentAdInfo;

namespace inline_content_ads {
class EventHandler final : public EventHandlerObserver {
 public:
  EventHandler();

  EventHandler(const EventHandler&) = delete;
  EventHandler& operator=(const EventHandler&) = delete;

  EventHandler(EventHandler&&) noexcept = delete;
  EventHandler& operator=(EventHandler&&) noexcept = delete;

  ~EventHandler() override;

  void AddObserver(EventHandlerObserver* observer);
  void RemoveObserver(EventHandlerObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::InlineContentAdEventType event_type);

 private:
  void OnGetForCreativeInstanceId(
      const std::string& placement_id,
      mojom::InlineContentAdEventType event_type,
      bool success,
      const std::string& creative_instance_id,
      const CreativeInlineContentAdInfo& creative_ad);

  void FireEvent(const InlineContentAdInfo& ad,
                 mojom::InlineContentAdEventType event_type);
  void OnGetAdEvents(const InlineContentAdInfo& ad,
                     mojom::InlineContentAdEventType event_type,
                     bool success,
                     const AdEventList& ad_events);
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

  base::WeakPtrFactory<EventHandler> weak_factory_{this};
};

}  // namespace inline_content_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
