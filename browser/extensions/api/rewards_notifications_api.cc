/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/extensions/api/rewards_notifications_api.h"

#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"

using brave_rewards::RewardsNotificationService;
using brave_rewards::RewardsServiceFactory;

namespace extensions {
namespace api {

RewardsNotificationsAddNotificationFunction::
~RewardsNotificationsAddNotificationFunction() {
}

ExtensionFunction::ResponseAction
RewardsNotificationsAddNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::AddNotification::Params> params(
      rewards_notifications::AddNotification::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->AddNotification(
        static_cast<RewardsNotificationService::RewardsNotificationType>(
            params->type),
        params->args, params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsDeleteNotificationFunction::
    ~RewardsNotificationsDeleteNotificationFunction() {}

ExtensionFunction::ResponseAction
RewardsNotificationsDeleteNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::DeleteNotification::Params> params(
      rewards_notifications::DeleteNotification::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->DeleteNotification(params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsDeleteAllNotificationsFunction::
~RewardsNotificationsDeleteAllNotificationsFunction() {
}

ExtensionFunction::ResponseAction
RewardsNotificationsDeleteAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->DeleteAllNotifications(false);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsGetNotificationFunction::
~RewardsNotificationsGetNotificationFunction() {
}

ExtensionFunction::ResponseAction
RewardsNotificationsGetNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::GetNotification::Params> params(
      rewards_notifications::GetNotification::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->GetNotification(params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsGetAllNotificationsFunction::
~RewardsNotificationsGetAllNotificationsFunction() {
}

ExtensionFunction::ResponseAction
RewardsNotificationsGetAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->GetAllNotifications();
  }
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
