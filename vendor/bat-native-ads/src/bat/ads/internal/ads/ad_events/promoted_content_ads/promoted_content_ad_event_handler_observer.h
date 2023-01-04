/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct PromotedContentAdInfo;

namespace promoted_content_ads {

class EventHandlerObserver : public base::CheckedObserver {
 public:
  // Invoked when the promoted content |ad| is served.
  virtual void OnPromotedContentAdServed(const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content |ad| is viewed.
  virtual void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content |ad| is clicked.
  virtual void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {}

  // Invoked when the promoted content |ad| event fails for |placement_id|,
  // |creative_instance_id| and |event_type|.
  virtual void OnPromotedContentAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::PromotedContentAdEventType event_type) {}
};

}  // namespace promoted_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_OBSERVER_H_
