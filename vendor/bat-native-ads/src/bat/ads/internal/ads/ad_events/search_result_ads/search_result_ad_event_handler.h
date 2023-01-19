/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/observer_list.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct SearchResultAdInfo;

namespace search_result_ads {

using FireAdEventHandlerCallback = base::OnceCallback<
    void(const bool, const std::string&, const mojom::SearchResultAdEventType)>;

class EventHandler final : public EventHandlerObserver {
 public:
  EventHandler();

  EventHandler(const EventHandler& other) = delete;
  EventHandler& operator=(const EventHandler& other) = delete;

  EventHandler(EventHandler&& other) noexcept = delete;
  EventHandler& operator=(EventHandler&& other) noexcept = delete;

  ~EventHandler() override;

  void AddObserver(EventHandlerObserver* observer);
  void RemoveObserver(EventHandlerObserver* observer);

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 mojom::SearchResultAdEventType event_type,
                 FireAdEventHandlerCallback callback) const;

 private:
  void FireEvent(const SearchResultAdInfo& ad,
                 mojom::SearchResultAdEventType event_type,
                 FireAdEventHandlerCallback callback) const;

  void FireViewedEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                       FireAdEventHandlerCallback callback) const;
  void OnSaveDeposits(mojom::SearchResultAdInfoPtr ad_mojom,
                      FireAdEventHandlerCallback callback,
                      bool success) const;
  void OnSaveConversions(const SearchResultAdInfo& ad,
                         FireAdEventHandlerCallback callback,
                         bool success) const;
  void OnGetAdEventsForViewedSearchResultAd(const SearchResultAdInfo& ad,
                                            FireAdEventHandlerCallback callback,
                                            bool success,
                                            const AdEventList& ad_events) const;

  void FireClickedEvent(const SearchResultAdInfo& ad,
                        FireAdEventHandlerCallback callback) const;
  void OnGetAdEventsForClickedSearchResultAd(
      const SearchResultAdInfo& ad,
      FireAdEventHandlerCallback callback,
      bool success,
      const AdEventList& ad_events) const;

  void FailedToFireEvent(const SearchResultAdInfo& ad,
                         mojom::SearchResultAdEventType event_type,
                         FireAdEventHandlerCallback callback) const;

  void NotifySearchResultAdEvent(const SearchResultAdInfo& ad,
                                 mojom::SearchResultAdEventType event_type,
                                 FireAdEventHandlerCallback callback) const;
  void NotifySearchResultAdServed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdViewed(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdClicked(const SearchResultAdInfo& ad) const;
  void NotifySearchResultAdEventFailed(
      const SearchResultAdInfo& ad,
      mojom::SearchResultAdEventType event_type,
      FireAdEventHandlerCallback callback) const;

  base::ObserverList<EventHandlerObserver> observers_;
};

}  // namespace search_result_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_
