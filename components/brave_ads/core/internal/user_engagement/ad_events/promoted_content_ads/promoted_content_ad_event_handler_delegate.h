/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

struct PromotedContentAdInfo;

class PromotedContentAdEventHandlerDelegate {
 public:
  // Invoked when the promoted content `ad` is served.
  virtual void OnDidFirePromotedContentAdServedEvent(
      const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content `ad` is viewed.
  virtual void OnDidFirePromotedContentAdViewedEvent(
      const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content `ad` is clicked.
  virtual void OnDidFirePromotedContentAdClickedEvent(
      const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content ad event fails for `placement_id`,
  // `creative_instance_id` and `mojom_ad_event_type`.
  virtual void OnFailedToFirePromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType mojom_ad_event_type) {}

 protected:
  virtual ~PromotedContentAdEventHandlerDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_DELEGATE_H_
