/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

struct SearchResultAdInfo;

class SearchResultAdEventHandlerDelegate {
 public:
  // Invoked when the search result `ad` is served.
  virtual void OnDidFireSearchResultAdServedEvent(
      const SearchResultAdInfo& ad) {}

  // Invoked when the search result `ad` is viewed.
  virtual void OnDidFireSearchResultAdViewedEvent(
      const SearchResultAdInfo& ad) {}

  // Invoked when the search result `ad` is clicked.
  virtual void OnDidFireSearchResultAdClickedEvent(
      const SearchResultAdInfo& ad) {}

  // Invoked when the search result `ad` event fails for `mojom_ad_event_type`.
  virtual void OnFailedToFireSearchResultAdEvent(
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType mojom_ad_event_type) {}

 protected:
  virtual ~SearchResultAdEventHandlerDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_H_
