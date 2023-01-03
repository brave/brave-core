/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_OBSERVER_H_

#include "base/observer_list_types.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct SearchResultAdInfo;

namespace search_result_ads {

class EventHandlerObserver : public base::CheckedObserver {
 public:
  // Invoked when the search result |ad| is served.
  virtual void OnSearchResultAdServed(const SearchResultAdInfo& ad) {}

  // Invoked when the search result |ad| is viewed.
  virtual void OnSearchResultAdViewed(const SearchResultAdInfo& ad) {}

  // Invoked when the search result |ad| is clicked.
  virtual void OnSearchResultAdClicked(const SearchResultAdInfo& ad) {}

  // Invoked when the search result |ad| event fails for |event_type|.
  virtual void OnSearchResultAdEventFailed(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type) {}
};

}  // namespace search_result_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_OBSERVER_H_
