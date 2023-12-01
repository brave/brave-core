/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/rewards_notifications_api.h"

#include <memory>
#include <optional>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"

using brave_rewards::RewardsNotificationService;
using brave_rewards::RewardsServiceFactory;

namespace extensions {
namespace api {

RewardsNotificationsAddNotificationFunction::
    ~RewardsNotificationsAddNotificationFunction() = default;

ExtensionFunction::ResponseAction
RewardsNotificationsAddNotificationFunction::Run() {
  std::optional<rewards_notifications::AddNotification::Params> params =
      rewards_notifications::AddNotification::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
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
    ~RewardsNotificationsDeleteNotificationFunction() = default;

ExtensionFunction::ResponseAction
RewardsNotificationsDeleteNotificationFunction::Run() {
  std::optional<rewards_notifications::DeleteNotification::Params> params =
      rewards_notifications::DeleteNotification::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->DeleteNotification(params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsDeleteAllNotificationsFunction::
    ~RewardsNotificationsDeleteAllNotificationsFunction() = default;

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
    ~RewardsNotificationsGetNotificationFunction() = default;

ExtensionFunction::ResponseAction
RewardsNotificationsGetNotificationFunction::Run() {
  std::optional<rewards_notifications::GetNotification::Params> params =
      rewards_notifications::GetNotification::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationService* rewards_notification_service =
      RewardsServiceFactory::GetForProfile(profile)->GetNotificationService();
  if (rewards_notification_service) {
    rewards_notification_service->GetNotification(params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsGetAllNotificationsFunction::
    ~RewardsNotificationsGetAllNotificationsFunction() = default;

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
