/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/extension_rewards_notification_service_observer.h"

#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"

namespace brave_rewards {

ExtensionRewardsNotificationServiceObserver::
ExtensionRewardsNotificationServiceObserver(
    RewardsNotificationService* notification_service,
    Profile* profile)
    : notification_service_(notification_service),
      profile_(profile) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  DCHECK(event_router);
  event_router->RegisterObserver(this,
      extensions::api::rewards_notifications::OnNotificationAdded::kEventName);
}

ExtensionRewardsNotificationServiceObserver::
~ExtensionRewardsNotificationServiceObserver() {
}

void ExtensionRewardsNotificationServiceObserver::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  auto notifications = notification_service_->GetAllNotifications();
  for (auto& notification : notifications) {
    OnNotificationAdded(notification_service_, notification.second);
  }
}

void ExtensionRewardsNotificationServiceObserver::OnNotificationAdded(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnNotificationAdded::Create(
            rewards_notification.id_, rewards_notification.type_,
            rewards_notification.timestamp_, rewards_notification.args_)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_NOTIFICATION_ADDED,
        extensions::api::rewards_notifications::OnNotificationAdded::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void ExtensionRewardsNotificationServiceObserver::OnNotificationDeleted(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnNotificationDeleted::Create(
            rewards_notification.id_, rewards_notification.type_,
            rewards_notification.timestamp_)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_NOTIFICATION_DELETED,
        extensions::api::rewards_notifications::OnNotificationDeleted::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void ExtensionRewardsNotificationServiceObserver::
    OnAllNotificationsDeleted(
        RewardsNotificationService* rewards_notification_service) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnAllNotificationsDeleted::Create()
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_ALL_NOTIFICATIONS_DELETED,
        extensions::api::rewards_notifications::OnAllNotificationsDeleted::
            kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void ExtensionRewardsNotificationServiceObserver::OnGetNotification(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnGetNotification::Create(
            rewards_notification.id_, rewards_notification.type_,
            rewards_notification.timestamp_, rewards_notification.args_)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_GET_NOTIFICATION,
        extensions::api::rewards_notifications::OnGetNotification::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void ExtensionRewardsNotificationServiceObserver::OnGetAllNotifications(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotificationsList&
        rewards_notifications_list) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::vector<extensions::api::rewards_notifications::OnGetAllNotifications::
                    NotificationsType>
        notifications_list;
    for (auto& item : rewards_notifications_list) {
      notifications_list.push_back(
          extensions::api::rewards_notifications::OnGetAllNotifications::
              NotificationsType());
      auto& notifications_type = notifications_list[notifications_list.size() - 1];
      notifications_type.id = item.id_;
      notifications_type.type = item.type_;
      notifications_type.timestamp = item.timestamp_;
      notifications_type.args = item.args_;
    }
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnGetAllNotifications::Create(
            notifications_list)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_GET_ALL_NOTIFICATIONS,
        extensions::api::rewards_notifications::OnGetAllNotifications::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

}  // namespace brave_rewards
