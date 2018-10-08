/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_OBSERVER_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_OBSERVER_

namespace brave_rewards {

class RewardsNotificationsService;

class RewardsNotificationsServiceObserver : public base::CheckedObserver {
 public:
  ~RewardsNotificationsServiceObserver() override {}

  virtual void OnNotificationAdded(
      RewardsNotificationsService* rewards_notifications_service,
      RewardsNotificationsService::RewardsNotification notification) {}
  virtual void OnNotificationDeleted(
      RewardsNotificationsService* rewards_notifications_service,
      RewardsNotificationsService::RewardsNotification notification) {}
  virtual void OnAllNotificationsDeleted(
      RewardsNotificationsService* rewards_notifications_service) {}
  virtual void OnGetNotification(
      RewardsNotificationsService* rewards_notifications_service,
      RewardsNotificationsService::RewardsNotification notification) {}
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_OBSERVER_
