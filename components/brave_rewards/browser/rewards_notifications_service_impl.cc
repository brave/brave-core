/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notifications_service_impl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/extensions/api/rewards_notifications.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_notifications_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/event_router.h"

namespace brave_rewards {

RewardsNotificationsServiceImpl::RewardsNotificationsServiceImpl(Profile* profile)
    : profile_(profile) {
}

RewardsNotificationsServiceImpl::~RewardsNotificationsServiceImpl() {
}

void RewardsNotificationsServiceImpl::Init() {
  ReadRewardsNotifications();
}

void RewardsNotificationsServiceImpl::Shutdown() {
  StoreRewardsNotifications();
  RewardsNotificationsService::Shutdown();
}

void RewardsNotificationsServiceImpl::AddNotification(
    RewardsNotificationType type,
    RewardsNotificationArgs args,
    RewardsNotificationID id) {
  DCHECK(type != REWARDS_NOTIFICATION_INVALID);
  if (id.empty())
    id = GenerateRewardsNotificationID();
  RewardsNotification rewards_notification(
      id, type, GenerateRewardsNotificationTimestamp(), std::move(args));
  rewards_notifications_[id] = rewards_notification;
  OnNotificationAdded(rewards_notification);
}

void RewardsNotificationsServiceImpl::DeleteNotification(RewardsNotificationID id) {
  DCHECK(!id.empty());
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
  DCHECK(!id.empty());
  if (rewards_notifications_.find(id) == rewards_notifications_.end())
    return;
  OnGetNotification(rewards_notifications_[id]);
}

void RewardsNotificationsServiceImpl::GetAllNotifications() {
  RewardsNotificationsList rewards_notifications_list;
  for (auto& item : rewards_notifications_) {
    rewards_notifications_list.push_back(item.second);
  }
  OnGetAllNotifications(rewards_notifications_list);
}

RewardsNotificationsServiceImpl::RewardsNotificationID
RewardsNotificationsServiceImpl::GenerateRewardsNotificationID() const {
  return base::StringPrintf(
      "%d", base::RandInt(0, std::numeric_limits<int32_t>::max()));
}

RewardsNotificationsServiceImpl::RewardsNotificationTimestamp
RewardsNotificationsServiceImpl::GenerateRewardsNotificationTimestamp() const {
  return base::Time::NowFromSystemTime().ToTimeT();
}

void RewardsNotificationsServiceImpl::ReadRewardsNotifications() {
  std::string json = profile_->GetPrefs()->GetString(kRewardsNotifications);
  if (json.empty())
    return;
  std::unique_ptr<base::ListValue> root =
      base::ListValue::From(base::JSONReader::Read(json));
  if (!root) {
    LOG(ERROR) << "Failed to deserialize rewards notifications on startup";
    return;
  }
  for (auto it = root->begin(); it != root->end(); ++it) {
    if (!it->is_dict())
      continue;
    base::DictionaryValue* dict_value;
    if (!it->GetAsDictionary(&dict_value))
      continue;
    std::string notification_id;
    int notification_type;
    int notification_timestamp;
    RewardsNotificationArgs notification_args;
    dict_value->GetString("id", &notification_id);
    dict_value->GetInteger("type", &notification_type);
    dict_value->GetInteger("timestamp", &notification_timestamp);

    base::ListValue* args;
    dict_value->GetList("args", &args);
    for (auto& arg : *args) {
      std::string arg_string = arg.GetString();
      notification_args.push_back(arg_string);
    }

    RewardsNotification notification(notification_id,
                                     static_cast<RewardsNotificationType>(notification_type),
                                     notification_timestamp,
                                     notification_args);
    rewards_notifications_[notification.id_] = notification;
  }
}

void RewardsNotificationsServiceImpl::StoreRewardsNotifications() {
  base::ListValue root;

  for (auto& item : rewards_notifications_) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("id", item.second.id_);
    dict->SetInteger("type", item.second.type_);
    dict->SetInteger("timestamp", item.second.timestamp_);
    auto args = std::make_unique<base::ListValue>();
    for (auto& arg : item.second.args_) {
      args->AppendString(arg);
    }
    dict->SetList("args", std::move(args));
    root.Append(std::move(dict));
  }

  std::string result;
  if (!base::JSONWriter::Write(root, &result)) {
    LOG(ERROR) << "Failed to serialize rewards notifications on shutdown";
    return;
  }

  profile_->GetPrefs()->SetString(kRewardsNotifications, result);
}

void RewardsNotificationsServiceImpl::TriggerOnNotificationAdded(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnNotificationAdded(this, rewards_notification);

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
            rewards_notification.timestamp_, rewards_notification.args_)
            .release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_REWARDS_GET_NOTIFICATION,
        extensions::api::rewards_notifications::OnGetNotification::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void RewardsNotificationsServiceImpl::TriggerOnGetAllNotifications(
    const RewardsNotificationsList& rewards_notifications_list) {
  for (auto& observer : observers_)
    observer.OnGetAllNotifications(this, rewards_notifications_list);

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

void RewardsNotificationsServiceImpl::OnNotificationAdded(
    const RewardsNotification& rewards_notification) {
  TriggerOnNotificationAdded(rewards_notification);
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

void RewardsNotificationsServiceImpl::OnGetAllNotifications(
    const RewardsNotificationsList& rewards_notifications_list) {
  TriggerOnGetAllNotifications(rewards_notifications_list);
}

}  // namespace brave_rewards
