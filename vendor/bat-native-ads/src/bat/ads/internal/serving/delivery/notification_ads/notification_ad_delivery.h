/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_DELIVERY_NOTIFICATION_ADS_NOTIFICATION_AD_DELIVERY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_DELIVERY_NOTIFICATION_ADS_NOTIFICATION_AD_DELIVERY_H_

namespace ads {

struct NotificationAdInfo;

namespace notification_ads {

class Delivery final {
 public:
  Delivery();
  ~Delivery();

  bool MaybeDeliverAd(const NotificationAdInfo& ad);
};

}  // namespace notification_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_DELIVERY_NOTIFICATION_ADS_NOTIFICATION_AD_DELIVERY_H_
