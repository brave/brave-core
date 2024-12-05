/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class NewTabPageAdEventHandlerDelegateMock
    : public NewTabPageAdEventHandlerDelegate {
 public:
  NewTabPageAdEventHandlerDelegateMock();

  NewTabPageAdEventHandlerDelegateMock(
      const NewTabPageAdEventHandlerDelegateMock&) = delete;
  NewTabPageAdEventHandlerDelegateMock& operator=(
      const NewTabPageAdEventHandlerDelegateMock&) = delete;

  ~NewTabPageAdEventHandlerDelegateMock() override;

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdServedEvent,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdViewedEvent,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdClickedEvent,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdMediaPlayEvent,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdMedia25Event,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireNewTabPageAdMedia100Event,
              (const NewTabPageAdInfo& ad));

  MOCK_METHOD(void,
              OnFailedToFireNewTabPageAdEvent,
              (const std::string& placement_id,
               const std::string& creative_instance_id,
               const mojom::NewTabPageAdEventType mojom_ad_event_type));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
