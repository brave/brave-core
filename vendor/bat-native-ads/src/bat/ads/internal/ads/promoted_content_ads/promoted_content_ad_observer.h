/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_OBSERVER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/mojom.h"

namespace ads {

struct PromotedContentAdInfo;

class PromotedContentAdObserver : public base::CheckedObserver {
 public:
  // Invoked when a promoted content ad is served
  virtual void OnPromotedContentAdServed(const PromotedContentAdInfo& ad) {}

  // Invoked when a promoted content ad is viewed
  virtual void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) {}

  // Invoked when a promoted content ad is clicked
  virtual void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) {}

  // Invoked when a promoted content ad event fails
  virtual void OnPromotedContentAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const PromotedContentAdEventType event_type) {}

 protected:
  ~PromotedContentAdObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_OBSERVER_H_
