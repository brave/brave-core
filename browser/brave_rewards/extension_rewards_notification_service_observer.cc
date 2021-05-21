/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/browser/brave_rewards/extension_rewards_notification_service_observer.h"

#include "brave/common/extensions/api/rewards_notifications.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"

namespace brave_rewards {

ExtensionRewardsNotificationServiceObserver::
ExtensionRewardsNotificationServiceObserver(Profile* profile)
    : profile_(profile) {
}

ExtensionRewardsNotificationServiceObserver::
~ExtensionRewardsNotificationServiceObserver() {
}

void ExtensionRewardsNotificationServiceObserver::OnNotificationAdded(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_REWARDS_NOTIFICATION_ADDED,
      extensions::api::rewards_notifications::OnNotificationAdded::kEventName,
      extensions::api::rewards_notifications::OnNotificationAdded::Create(
          rewards_notification.id_, rewards_notification.type_,
          rewards_notification.timestamp_, rewards_notification.args_)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsNotificationServiceObserver::OnNotificationDeleted(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_REWARDS_NOTIFICATION_DELETED,
      extensions::api::rewards_notifications::OnNotificationDeleted::kEventName,
      extensions::api::rewards_notifications::OnNotificationDeleted::Create(
          rewards_notification.id_, rewards_notification.type_,
          rewards_notification.timestamp_)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsNotificationServiceObserver::
    OnAllNotificationsDeleted(
        RewardsNotificationService* rewards_notification_service) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_REWARDS_ALL_NOTIFICATIONS_DELETED,
      extensions::api::rewards_notifications::OnAllNotificationsDeleted::
          kEventName,
      extensions::api::rewards_notifications::OnAllNotificationsDeleted::
          Create()));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsNotificationServiceObserver::OnGetNotification(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotification&
        rewards_notification) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_REWARDS_GET_NOTIFICATION,
      extensions::api::rewards_notifications::OnGetNotification::kEventName,
      extensions::api::rewards_notifications::OnGetNotification::Create(
          rewards_notification.id_, rewards_notification.type_,
          rewards_notification.timestamp_, rewards_notification.args_)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsNotificationServiceObserver::OnGetAllNotifications(
    RewardsNotificationService* rewards_notification_service,
    const RewardsNotificationService::RewardsNotificationsList&
        rewards_notifications_list) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::vector<extensions::api::rewards_notifications::OnGetAllNotifications::
                  NotificationsType>
      notifications_list;
  for (auto& item : rewards_notifications_list) {
    notifications_list.push_back(
        extensions::api::rewards_notifications::OnGetAllNotifications::
            NotificationsType());
    auto& notifications_type =
        notifications_list[notifications_list.size() - 1];
    notifications_type.id = item.id_;
    notifications_type.type = item.type_;
    notifications_type.timestamp = item.timestamp_;
    notifications_type.args = item.args_;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_REWARDS_GET_ALL_NOTIFICATIONS,
      extensions::api::rewards_notifications::OnGetAllNotifications::kEventName,
      extensions::api::rewards_notifications::OnGetAllNotifications::Create(
          notifications_list)));
  event_router->BroadcastEvent(std::move(event));
}

}  // namespace brave_rewards
