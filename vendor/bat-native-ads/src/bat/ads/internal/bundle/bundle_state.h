/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_BUNDLE_BUNDLE_STATE_H_
#define BAT_ADS_INTERNAL_BUNDLE_BUNDLE_STATE_H_

#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/bundle/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

struct BundleState {
  BundleState();
  BundleState(
      const BundleState& state);
  ~BundleState();

  CreativeAdNotificationList creative_ad_notifications;
  CreativeNewTabPageAdList creative_new_tab_page_ads;
  CreativePromotedContentAdList creative_promoted_content_ads;
  ConversionList conversions;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_BUNDLE_BUNDLE_STATE_H_
