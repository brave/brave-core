/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class SearchResultAdEventHandlerDelegateMock
    : public SearchResultAdEventHandlerDelegate {
 public:
  SearchResultAdEventHandlerDelegateMock();

  SearchResultAdEventHandlerDelegateMock(
      const SearchResultAdEventHandlerDelegateMock&) = delete;
  SearchResultAdEventHandlerDelegateMock& operator=(
      const SearchResultAdEventHandlerDelegateMock&) = delete;

  ~SearchResultAdEventHandlerDelegateMock() override;

  MOCK_METHOD(void,
              OnDidFireSearchResultAdServedEvent,
              (const SearchResultAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireSearchResultAdViewedEvent,
              (const SearchResultAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireSearchResultAdClickedEvent,
              (const SearchResultAdInfo& ad));

  MOCK_METHOD(void,
              OnFailedToFireSearchResultAdEvent,
              (const SearchResultAdInfo& ad,
               mojom::SearchResultAdEventType mojom_ad_event_type));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_SEARCH_RESULT_ADS_SEARCH_RESULT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
