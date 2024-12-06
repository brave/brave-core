/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct InlineContentAdInfo;

class InlineContentAdEventHandlerDelegate {
 public:
  // Invoked when the inline content `ad` is served.
  virtual void OnDidFireInlineContentAdServedEvent(
      const InlineContentAdInfo& ad) {}

  // Invoked when the inline content `ad` is viewed.
  virtual void OnDidFireInlineContentAdViewedEvent(
      const InlineContentAdInfo& ad) {}

  // Invoked when the inline content `ad` is clicked.
  virtual void OnDidFireInlineContentAdClickedEvent(
      const InlineContentAdInfo& ad) {}

  // Invoked when the inline content ad event fails for `placement_id`,
  // `creative_instance_id` and `mojom_ad_event_type`.
  virtual void OnFailedToFireInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type) {}

 protected:
  virtual ~InlineContentAdEventHandlerDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_
