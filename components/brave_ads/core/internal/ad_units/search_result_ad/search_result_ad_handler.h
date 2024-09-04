/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class SiteVisit;
struct SearchResultAdInfo;

class SearchResultAdHandler final : public SearchResultAdEventHandlerDelegate {
 public:
  explicit SearchResultAdHandler(SiteVisit& site_visit);

  SearchResultAdHandler(const SearchResultAdHandler&) = delete;
  SearchResultAdHandler& operator=(const SearchResultAdHandler&) = delete;

  SearchResultAdHandler(SearchResultAdHandler&&) noexcept = delete;
  SearchResultAdHandler& operator=(SearchResultAdHandler&&) noexcept = delete;

  ~SearchResultAdHandler() override;

  static void DeferTriggeringAdViewedEventForTesting();

  // You must call this if `DeferTriggeringAdViewedEventForTesting` is called.
  static void TriggerDeferredAdViewedEventForTesting();

  void TriggerEvent(mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    mojom::SearchResultAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback);

 private:
  void FireServedEventCallback(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      TriggerAdEventCallback callback,
      bool success,
      const std::string& placement_id,
      mojom::SearchResultAdEventType mojom_ad_event_type);

  void MaybeTriggerDeferredAdViewedEvent(TriggerAdEventCallback callback);
  void FireAdViewedEventCallback(
      TriggerAdEventCallback callback,
      bool success,
      const std::string& placement_id,
      mojom::SearchResultAdEventType mojom_ad_event_type);

  // SearchResultAdEventHandlerDelegate:
  void OnDidFireSearchResultAdServedEvent(
      const SearchResultAdInfo& ad) override;
  void OnDidFireSearchResultAdViewedEvent(
      const SearchResultAdInfo& ad) override;
  void OnDidFireSearchResultAdClickedEvent(
      const SearchResultAdInfo& ad) override;

  const raw_ref<SiteVisit> site_visit_;

  SearchResultAdEventHandler event_handler_;

  base::circular_deque<mojom::CreativeSearchResultAdInfoPtr>
      ad_viewed_event_queue_;

  bool trigger_ad_viewed_event_in_progress_ = false;

  base::WeakPtrFactory<SearchResultAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_HANDLER_H_
