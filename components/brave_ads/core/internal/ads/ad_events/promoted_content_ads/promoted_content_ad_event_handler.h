/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_observer.h"

namespace brave_ads {

struct CreativePromotedContentAdInfo;
struct PromotedContentAdInfo;

namespace promoted_content_ads {

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
                 mojom::PromotedContentAdEventType event_type);

 private:
  void OnGetForCreativeInstanceId(
      const std::string& placement_id,
      mojom::PromotedContentAdEventType event_type,
      bool success,
      const std::string& creative_instance_id,
      const CreativePromotedContentAdInfo& creative_ad);

  void FireEvent(const PromotedContentAdInfo& ad,
                 mojom::PromotedContentAdEventType event_type);
  void OnGetAdEvents(const PromotedContentAdInfo& ad,
                     mojom::PromotedContentAdEventType event_type,
                     bool success,
                     const AdEventList& ad_events);
  void FailedToFireEvent(const std::string& placement_id,
                         const std::string& creative_instance_id,
                         mojom::PromotedContentAdEventType event_type) const;

  void NotifyPromotedContentAdEvent(
      const PromotedContentAdInfo& ad,
      mojom::PromotedContentAdEventType event_type) const;
  void NotifyPromotedContentAdServed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdViewed(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdClicked(const PromotedContentAdInfo& ad) const;
  void NotifyPromotedContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type) const;

  base::ObserverList<EventHandlerObserver> observers_;

  base::WeakPtrFactory<EventHandler> weak_factory_{this};
};

}  // namespace promoted_content_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_
