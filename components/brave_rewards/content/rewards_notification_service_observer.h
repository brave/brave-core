/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_NOTIFICATION_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_NOTIFICATION_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"
#include "brave/components/brave_rewards/content/rewards_notification_service.h"

namespace brave_rewards {

class RewardsNotificationService;

class RewardsNotificationServiceObserver : public base::CheckedObserver {
 public:
  ~RewardsNotificationServiceObserver() override {}

  virtual void OnNotificationAdded(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) {}
  virtual void OnNotificationDeleted(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) {}
  virtual void OnAllNotificationsDeleted(
      RewardsNotificationService* rewards_notification_service) {}
  virtual void OnGetNotification(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotification& notification) {}
  virtual void OnGetAllNotifications(
      RewardsNotificationService* rewards_notification_service,
      const RewardsNotificationService::RewardsNotificationsList&
          notifications_list) {}
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_NOTIFICATION_SERVICE_OBSERVER_H_
