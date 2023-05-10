/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler_delegate.h"

namespace brave_ads {

struct SearchResultAdInfo;

using FireAdEventHandlerCallback =
    base::OnceCallback<void(bool success,
                            const std::string& placement_id,
                            const mojom::SearchResultAdEventType event_type)>;

class SearchResultAdEventHandler final
    : public SearchResultAdEventHandlerDelegate {
 public:
  SearchResultAdEventHandler();

  SearchResultAdEventHandler(const SearchResultAdEventHandler&) = delete;
  SearchResultAdEventHandler& operator=(const SearchResultAdEventHandler&) =
      delete;

  SearchResultAdEventHandler(SearchResultAdEventHandler&&) noexcept = delete;
  SearchResultAdEventHandler& operator=(SearchResultAdEventHandler&&) noexcept =
      delete;

  ~SearchResultAdEventHandler() override;

  void SetDelegate(SearchResultAdEventHandlerDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 mojom::SearchResultAdEventType event_type,
                 FireAdEventHandlerCallback callback) const;

 private:
  void FireEvent(const SearchResultAdInfo& ad,
                 mojom::SearchResultAdEventType event_type,
                 FireAdEventHandlerCallback callback) const;

  void FireViewedEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                       FireAdEventHandlerCallback callback) const;
  void SaveDepositsCallback(mojom::SearchResultAdInfoPtr ad_mojom,
                            FireAdEventHandlerCallback callback,
                            bool success) const;
  void SaveConversionsCallback(const SearchResultAdInfo& ad,
                               FireAdEventHandlerCallback callback,
                               bool success) const;
  void GetAdEventsForViewedSearchResultAdCallback(
      const SearchResultAdInfo& ad,
      FireAdEventHandlerCallback callback,
      bool success,
      const AdEventList& ad_events) const;

  void FireClickedEvent(const SearchResultAdInfo& ad,
                        FireAdEventHandlerCallback callback) const;
  void GetAdEventsForClickedSearchResultAdCallback(
      const SearchResultAdInfo& ad,
      FireAdEventHandlerCallback callback,
      bool success,
      const AdEventList& ad_events) const;

  void SuccessfullyFiredEvent(const SearchResultAdInfo& ad,
                              mojom::SearchResultAdEventType event_type,
                              FireAdEventHandlerCallback callback) const;
  void FailedToFireEvent(const SearchResultAdInfo& ad,
                         mojom::SearchResultAdEventType event_type,
                         FireAdEventHandlerCallback callback) const;

  raw_ptr<SearchResultAdEventHandlerDelegate> delegate_ = nullptr;

  base::WeakPtrFactory<SearchResultAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_H_
