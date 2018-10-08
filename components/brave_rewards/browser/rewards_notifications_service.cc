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

void RewardsNotificationsService::AddObserver(RewardsNotificationsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void RewardsNotificationsService::RemoveObserver(RewardsNotificationsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_rewards
