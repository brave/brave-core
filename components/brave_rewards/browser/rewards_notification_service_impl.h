/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "extensions/buildflags/buildflags.h"

class Profile;

namespace brave_rewards {

class ExtensionRewardsNotificationServiceObserver;

class RewardsNotificationServiceImpl
    : public RewardsNotificationService,
      public RewardsServiceObserver,
      public base::SupportsWeakPtr<RewardsNotificationServiceImpl> {
 public:
  explicit RewardsNotificationServiceImpl(Profile* profile);
  ~RewardsNotificationServiceImpl() override;

  void AddNotification(RewardsNotificationType type,
                       RewardsNotificationArgs args,
                       RewardsNotificationID id = "",
                       bool only_once = false) override;
  void DeleteNotification(RewardsNotificationID id) override;
  void DeleteAllNotifications() override;
  void GetNotification(RewardsNotificationID id) override;
  void GetNotifications() override;
  const RewardsNotificationsMap& GetAllNotifications() const override;

  void ReadRewardsNotificationsJSON() override;
  void ReadRewardsNotifications(const base::Value::ListStorage& root);
  void StoreRewardsNotifications() override;

 private:
  bool IsUGPGrant(const std::string& grant_type);
  std::string GetGrantIdPrefix(const std::string& grant_type);

  // RewardsServiceObserver impl
  void OnGrant(RewardsService* rewards_service,
               unsigned int error_code,
               Grant properties) override;
  void OnGrantFinish(RewardsService* rewards_service,
                     unsigned int result,
                     brave_rewards::Grant grant) override;
  void OnReconcileComplete(RewardsService* rewards_service,
                           unsigned int result,
                           const std::string& viewing_id,
                           const std::string& category,
                           const std::string& probi) override;

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
  RewardsNotificationsMap rewards_notifications_;
  std::vector<RewardsNotificationID> rewards_notifications_displayed_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsNotificationServiceObserver>
      extension_rewards_notification_service_observer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(RewardsNotificationServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_IMPL_H_
