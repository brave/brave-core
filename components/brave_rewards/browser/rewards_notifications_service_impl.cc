/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notifications_service_impl.h"

#include "base/rand_util.h"
#include "base/time/time.h"
#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"

namespace brave_rewards {

RewardsNotificationsServiceImpl::RewardsNotificationsServiceImpl(Profile* profile)
    : profile_(profile) {
}

RewardsNotificationsServiceImpl::~RewardsNotificationsServiceImpl() {
}

void RewardsNotificationsServiceImpl::Init() {
}

void RewardsNotificationsServiceImpl::Shutdown() {
  RewardsNotificationsService::Shutdown();
}

void RewardsNotificationsServiceImpl::AddNotification(
    RewardsNotificationType type,
    RewardsNotificationArgs args) {
  RewardsNotificationID id = GenerateRewardsNotificationID();
  RewardsNotification rewards_notification(
      id, type, GenerateRewardsNotificationTimestamp());
  rewards_notifications_[id] = rewards_notification;
  OnNotificationAdded(rewards_notification, std::move(args));
}

void RewardsNotificationsServiceImpl::DeleteNotification(RewardsNotificationID id) {
  if (rewards_notifications_.find(id) == rewards_notifications_.end())
    return;
  RewardsNotification rewards_notification = rewards_notifications_[id];
  rewards_notifications_.erase(id);
  OnNotificationDeleted(rewards_notification);
}

void RewardsNotificationsServiceImpl::DeleteAllNotifications() {
  rewards_notifications_.clear();
  OnAllNotificationsDeleted();
}

void RewardsNotificationsServiceImpl::GetNotification(RewardsNotificationID id) {
  if (rewards_notifications_.find(id) == rewards_notifications_.end())
    return;
  OnGetNotification(rewards_notifications_[id]);
}

RewardsNotificationsServiceImpl::RewardsNotificationID
RewardsNotificationsServiceImpl::GenerateRewardsNotificationID() const {
  return base::RandUint64();
}

RewardsNotificationsServiceImpl::RewardsNotificationTimestamp
RewardsNotificationsServiceImpl::GenerateRewardsNotificationTimestamp() const {
  return base::Time::Now().ToDeltaSinceWindowsEpoch().InSeconds();
}

void RewardsNotificationsServiceImpl::TriggerOnNotificationAdded(
    const RewardsNotification& rewards_notification,
    const RewardsNotificationArgs& notification_args) {
  for (auto& observer : observers_)
    observer.OnNotificationAdded(this, rewards_notification, notification_args);

  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnNotificationAdded::Create(
            rewards_notification.id_, rewards_notification.type_,
            rewards_notification.timestamp_, notification_args)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_NOTIFICATION_ADDED,
        extensions::api::rewards_notifications::OnNotificationAdded::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void RewardsNotificationsServiceImpl::TriggerOnNotificationDeleted(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnNotificationDeleted(this, rewards_notification);

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

void RewardsNotificationsServiceImpl::TriggerOnAllNotificationsDeleted() {
  for (auto& observer : observers_)
    observer.OnAllNotificationsDeleted(this);

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

void RewardsNotificationsServiceImpl::TriggerOnGetNotification(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnGetNotification(this, rewards_notification);

  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::rewards_notifications::OnGetNotification::Create(
            rewards_notification.id_, rewards_notification.type_,
            rewards_notification.timestamp_)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_GET_NOTIFICATION,
        extensions::api::rewards_notifications::OnGetNotification::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void RewardsNotificationsServiceImpl::OnNotificationAdded(
    const RewardsNotification& rewards_notification,
    const RewardsNotificationArgs& args) {
  TriggerOnNotificationAdded(rewards_notification, args);
}

void RewardsNotificationsServiceImpl::OnNotificationDeleted(
    const RewardsNotification& rewards_notification) {
  TriggerOnNotificationDeleted(rewards_notification);
}

void RewardsNotificationsServiceImpl::OnAllNotificationsDeleted() {
  TriggerOnAllNotificationsDeleted();
}

void RewardsNotificationsServiceImpl::OnGetNotification(
    const RewardsNotification& rewards_notification) {
  TriggerOnGetNotification(rewards_notification);
}

}  // namespace brave_rewards
