/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

struct NotificationAdInfo;

class NotificationAdServingObserver : public base::CheckedObserver {
 public:
  // Invoked when a notification ad is served
  virtual void OnDidServeNotificationAd(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad fails to serve
  virtual void OnFailedToServeNotificationAd() {}

 protected:
  ~NotificationAdServingObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_OBSERVER_H_
