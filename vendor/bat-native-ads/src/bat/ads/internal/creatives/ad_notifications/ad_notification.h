/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/creatives/ad_notifications/ad_notification_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct AdNotificationInfo;

class AdNotification final : public AdNotificationObserver {
 public:
  AdNotification();
  ~AdNotification() override;

  void AddObserver(AdNotificationObserver* observer);
  void RemoveObserver(AdNotificationObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const mojom::AdNotificationEventType event_type);

 private:
  void NotifyAdNotificationEvent(
      const AdNotificationInfo& ad,
      const mojom::AdNotificationEventType event_type) const;
  void NotifyAdNotificationServed(const AdNotificationInfo& ad) const;
  void NotifyAdNotificationViewed(const AdNotificationInfo& ad) const;
  void NotifyAdNotificationClicked(const AdNotificationInfo& ad) const;
  void NotifyAdNotificationDismissed(const AdNotificationInfo& ad) const;
  void NotifyAdNotificationTimedOut(const AdNotificationInfo& ad) const;

  void NotifyAdNotificationEventFailed(
      const std::string& placement_id,
      const mojom::AdNotificationEventType event_type) const;

  base::ObserverList<AdNotificationObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_AD_NOTIFICATIONS_AD_NOTIFICATION_H_
