/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_HANDLER_H_

#include <memory>
#include <string>

#include "base/containers/circular_deque.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler_observer.h"

namespace brave_ads {

namespace search_result_ads {
class EventHandler;
}  // namespace search_result_ads

class Account;
class Transfer;
struct SearchResultAdInfo;

class SearchResultAd final : public search_result_ads::EventHandlerObserver {
 public:
  SearchResultAd(Account* account, Transfer* transfer);

  SearchResultAd(const SearchResultAd& other) = delete;
  SearchResultAd& operator=(const SearchResultAd& other) = delete;

  SearchResultAd(SearchResultAd&& other) noexcept = delete;
  SearchResultAd& operator=(SearchResultAd&& other) noexcept = delete;

  ~SearchResultAd() override;

  void TriggerEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                    mojom::SearchResultAdEventType event_type);

  static void DeferTriggeringOfAdViewedEventForTesting();
  static void TriggerDeferredAdViewedEventForTesting();

 private:
  void MaybeTriggerAdViewedEventFromQueue();
  void OnFireAdViewedEvent(bool success,
                           const std::string& placement_id,
                           mojom::SearchResultAdEventType event_type);

  // search_result_ads::EventHandlerObserver:
  void OnSearchResultAdViewed(const SearchResultAdInfo& ad) override;
  void OnSearchResultAdClicked(const SearchResultAdInfo& ad) override;

  std::unique_ptr<search_result_ads::EventHandler> event_handler_;

  base::circular_deque<mojom::SearchResultAdInfoPtr> ad_viewed_event_queue_;

  bool trigger_ad_viewed_event_in_progress_ = false;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED

  base::WeakPtrFactory<SearchResultAd> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SEARCH_RESULT_AD_HANDLER_H_
