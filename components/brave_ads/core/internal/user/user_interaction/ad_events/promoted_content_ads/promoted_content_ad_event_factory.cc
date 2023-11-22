/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_clicked.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_served.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_viewed.h"

namespace brave_ads {

std::unique_ptr<AdEventInterface<PromotedContentAdInfo>>
PromotedContentAdEventFactory::Build(
    const mojom::PromotedContentAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::PromotedContentAdEventType::kServed: {
      return std::make_unique<PromotedContentAdEventServed>();
    }

    case mojom::PromotedContentAdEventType::kViewed: {
      return std::make_unique<PromotedContentAdEventViewed>();
    }

    case mojom::PromotedContentAdEventType::kClicked: {
      return std::make_unique<PromotedContentAdEventClicked>();
    }
  }
}

}  // namespace brave_ads
