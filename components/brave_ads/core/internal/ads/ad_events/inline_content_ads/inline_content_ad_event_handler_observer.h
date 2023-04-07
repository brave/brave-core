/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"

namespace brave_ads {

struct InlineContentAdInfo;

namespace inline_content_ads {

class EventHandlerObserver : public base::CheckedObserver {
 public:
  // Invoked when the inline content |ad| is served.
  virtual void OnInlineContentAdServed(const InlineContentAdInfo& ad) {}

  // Invoked when the inline content |ad| is viewed.
  virtual void OnInlineContentAdViewed(const InlineContentAdInfo& ad) {}

  // Invoked when the inline content |ad| is clicked.
  virtual void OnInlineContentAdClicked(const InlineContentAdInfo& ad) {}

  // Invoked when the inline content |ad| event fails for |placement_id|,
  // |creative_instance_id| and |event_type|.
  virtual void OnInlineContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::InlineContentAdEventType event_type) {}
};

}  // namespace inline_content_ads
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_
