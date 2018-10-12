/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_IMPL_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_IMPL_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/browser/rewards_notifications_service.h"

class Profile;

namespace brave_rewards {

class RewardsNotificationsServiceImpl
    : public RewardsNotificationsService,
      public base::SupportsWeakPtr<RewardsNotificationsServiceImpl> {
 public:
  RewardsNotificationsServiceImpl(Profile* profile);
  ~RewardsNotificationsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  void Init();
  void AddNotification(RewardsNotificationType type,
                       RewardsNotificationArgs args) override;
  void DeleteNotification(RewardsNotificationID id) override;
  void DeleteAllNotifications() override;
  void GetNotification(RewardsNotificationID id) override;

private:
 void TriggerOnNotificationAdded(
     const RewardsNotification& rewards_notification,
     const RewardsNotificationArgs& args);
 void TriggerOnNotificationDeleted(
     const RewardsNotification& rewards_notification);
 void TriggerOnAllNotificationsDeleted();
 void TriggerOnGetNotification(const RewardsNotification& rewards_notification);

 void OnNotificationAdded(const RewardsNotification& rewards_notification,
                          const RewardsNotificationArgs& args);
 void OnNotificationDeleted(const RewardsNotification& rewards_notification);
 void OnAllNotificationsDeleted();
 void OnGetNotification(const RewardsNotification& rewards_notification);

 RewardsNotificationID GenerateRewardsNotificationID() const;
 RewardsNotificationTimestamp GenerateRewardsNotificationTimestamp() const;

 Profile* profile_;
 std::map<RewardsNotificationID, RewardsNotification> rewards_notifications_;

 DISALLOW_COPY_AND_ASSIGN(RewardsNotificationsServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_IMPL_
