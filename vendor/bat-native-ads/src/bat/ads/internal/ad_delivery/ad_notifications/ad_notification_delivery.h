/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DELIVERY_AD_NOTIFICATIONS_AD_NOTIFICATION_DELIVERY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DELIVERY_AD_NOTIFICATIONS_AD_NOTIFICATION_DELIVERY_H_

namespace ads {

struct AdNotificationInfo;

namespace ad_notifications {

class AdDelivery {
 public:
  AdDelivery();
  ~AdDelivery();

  bool MaybeDeliverAd(const AdNotificationInfo& ad);
};

}  // namespace ad_notifications
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DELIVERY_AD_NOTIFICATIONS_AD_NOTIFICATION_DELIVERY_H_
