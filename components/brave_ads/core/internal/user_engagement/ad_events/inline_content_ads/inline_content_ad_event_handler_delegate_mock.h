/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class InlineContentAdEventHandlerDelegateMock
    : public InlineContentAdEventHandlerDelegate {
 public:
  InlineContentAdEventHandlerDelegateMock();

  InlineContentAdEventHandlerDelegateMock(
      const InlineContentAdEventHandlerDelegateMock&) = delete;
  InlineContentAdEventHandlerDelegateMock& operator=(
      const InlineContentAdEventHandlerDelegateMock&) = delete;

  ~InlineContentAdEventHandlerDelegateMock() override;

  MOCK_METHOD(void,
              OnDidFireInlineContentAdServedEvent,
              (const InlineContentAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireInlineContentAdViewedEvent,
              (const InlineContentAdInfo& ad));

  MOCK_METHOD(void,
              OnDidFireInlineContentAdClickedEvent,
              (const InlineContentAdInfo& ad));

  MOCK_METHOD(void,
              OnFailedToFireInlineContentAdEvent,
              (const std::string& placement_id,
               const std::string& creative_instance_id,
               const mojom::InlineContentAdEventType mojom_ad_event_type));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
