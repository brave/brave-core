/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

struct AdNotificationInfo;

class AdNotificationServingObserver : public base::CheckedObserver {
 public:
  // Invoked when an ad notification is served
  virtual void OnDidServeAdNotification(const AdNotificationInfo& ad) {}

  // Invoked when an ad notification fails to serve
  virtual void OnFailedToServeAdNotification() {}

 protected:
  ~AdNotificationServingObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_AD_NOTIFICATIONS_AD_NOTIFICATION_SERVING_OBSERVER_H_
