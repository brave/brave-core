/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notifications_service.h"

#include "brave/components/brave_rewards/browser/rewards_notifications_service_observer.h"

namespace brave_rewards {

RewardsNotificationsService::RewardsNotificationsService() {
}

RewardsNotificationsService::~RewardsNotificationsService() {
}

RewardsNotificationsService::RewardsNotification::RewardsNotification() {}

RewardsNotificationsService::RewardsNotification::RewardsNotification(const RewardsNotificationsService::RewardsNotification& prop) = default;

RewardsNotificationsService::RewardsNotification::RewardsNotification(
    RewardsNotificationsService::RewardsNotificationID id,
    RewardsNotificationsService::RewardsNotificationType type,
    RewardsNotificationsService::RewardsNotificationTimestamp timestamp,
    RewardsNotificationsService::RewardsNotificationArgs args)
    : id_(id), type_(type), timestamp_(timestamp), args_(args) {}

RewardsNotificationsService::RewardsNotification::~RewardsNotification() {}

void RewardsNotificationsService::AddObserver(RewardsNotificationsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void RewardsNotificationsService::RemoveObserver(RewardsNotificationsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_rewards
