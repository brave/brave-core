/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct NotificationAdInfo;

class NotificationAdServingDelegate {
 public:
  // Invoked when an opportunity arises to serve a notification ad.
  virtual void OnOpportunityAroseToServeNotificationAd(
      const SegmentList& segments) {}

  // Invoked when a notification ad is served.
  virtual void OnDidServeNotificationAd(const NotificationAdInfo& ad) {}

  // Invoked when a notification ad fails to serve.
  virtual void OnFailedToServeNotificationAd() {}

 protected:
  virtual ~NotificationAdServingDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_H_
