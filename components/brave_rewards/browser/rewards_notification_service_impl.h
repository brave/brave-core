/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "extensions/buildflags/buildflags.h"

class Profile;

namespace brave_rewards {

class ExtensionRewardsNotificationServiceObserver;

class RewardsNotificationServiceImpl
    : public RewardsNotificationService,
      public base::SupportsWeakPtr<RewardsNotificationServiceImpl> {
 public:
  RewardsNotificationServiceImpl(Profile* profile);
  ~RewardsNotificationServiceImpl() override;

  void AddNotification(RewardsNotificationType type,
                       RewardsNotificationArgs args,
                       RewardsNotificationID id = "") override;
  void DeleteNotification(RewardsNotificationID id) override;
  void DeleteAllNotifications() override;
  void GetNotification(RewardsNotificationID id) override;
  void GetAllNotifications() override;

  void ReadRewardsNotifications() override;
  void StoreRewardsNotifications() override;

 private:
  void TriggerOnNotificationAdded(
      const RewardsNotification& rewards_notification);
  void TriggerOnNotificationDeleted(
      const RewardsNotification& rewards_notification);
  void TriggerOnAllNotificationsDeleted();
  void TriggerOnGetNotification(
      const RewardsNotification& rewards_notification);
  void TriggerOnGetAllNotifications(
      const RewardsNotificationsList& rewards_notifications_list);

  void OnNotificationAdded(const RewardsNotification& rewards_notification);
  void OnNotificationDeleted(const RewardsNotification& rewards_notification);
  void OnAllNotificationsDeleted();
  void OnGetNotification(const RewardsNotification& rewards_notification);
  void OnGetAllNotifications(
      const RewardsNotificationsList& rewards_notifications_list);

  RewardsNotificationID GenerateRewardsNotificationID() const;
  RewardsNotificationTimestamp GenerateRewardsNotificationTimestamp() const;

  Profile* profile_;
  std::map<RewardsNotificationID, RewardsNotification> rewards_notifications_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsNotificationServiceObserver>
      extension_rewards_notification_service_observer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(RewardsNotificationServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_
