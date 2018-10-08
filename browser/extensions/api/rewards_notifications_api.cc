/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/rewards_notifications_api.h"

#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service_factory.h"
#include "chrome/browser/profiles/profile.h"

using brave_rewards::RewardsNotificationsService;
using brave_rewards::RewardsNotificationsServiceFactory;

namespace extensions {
namespace api {

RewardsNotificationsAddNotificationFunction::~RewardsNotificationsAddNotificationFunction() {
}

ExtensionFunction::ResponseAction RewardsNotificationsAddNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::AddNotification::Params> params(
      rewards_notifications::AddNotification::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationsService* rewards_notifications_service =
      RewardsNotificationsServiceFactory::GetForProfile(profile);
  if (rewards_notifications_service) {
    rewards_notifications_service->AddNotification(
        static_cast<RewardsNotificationsService::RewardsNotificationType>(
            params->type));
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsDeleteNotificationFunction::
    ~RewardsNotificationsDeleteNotificationFunction() {}

ExtensionFunction::ResponseAction RewardsNotificationsDeleteNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::DeleteNotification::Params> params(
      rewards_notifications::DeleteNotification::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationsService* rewards_notifications_service =
      RewardsNotificationsServiceFactory::GetForProfile(profile);
  if (rewards_notifications_service) {
    rewards_notifications_service->DeleteNotification(params->id);
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsDeleteAllNotificationsFunction::~RewardsNotificationsDeleteAllNotificationsFunction() {
}

ExtensionFunction::ResponseAction RewardsNotificationsDeleteAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationsService* rewards_notifications_service =
      RewardsNotificationsServiceFactory::GetForProfile(profile);
  if (rewards_notifications_service) {
    rewards_notifications_service->DeleteAllNotifications();
  }
  return RespondNow(NoArguments());
}

RewardsNotificationsGetNotificationFunction::~RewardsNotificationsGetNotificationFunction() {
}

ExtensionFunction::ResponseAction RewardsNotificationsGetNotificationFunction::Run() {
  std::unique_ptr<rewards_notifications::GetNotification::Params> params(
      rewards_notifications::GetNotification::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsNotificationsService* rewards_notifications_service =
      RewardsNotificationsServiceFactory::GetForProfile(profile);
  if (rewards_notifications_service) {
    rewards_notifications_service->GetNotification(params->id);
  }
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
