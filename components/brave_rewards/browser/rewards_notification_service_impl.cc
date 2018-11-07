/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_rewards/browser/extension_rewards_notification_service_observer.h"
#endif

namespace brave_rewards {

RewardsNotificationServiceImpl::RewardsNotificationServiceImpl(Profile* profile)
    : profile_(profile)
#if BUILDFLAG(ENABLE_EXTENSIONS)
      , extension_rewards_notification_service_observer_(
          std::make_unique<ExtensionRewardsNotificationServiceObserver>(
              profile))
#endif
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
  AddObserver(extension_rewards_notification_service_observer_.get());
#endif
  ReadRewardsNotifications();
}

RewardsNotificationServiceImpl::~RewardsNotificationServiceImpl() {
  StoreRewardsNotifications();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  RemoveObserver(extension_rewards_notification_service_observer_.get());
#endif
}

void RewardsNotificationServiceImpl::AddNotification(
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

void RewardsNotificationServiceImpl::DeleteNotification(RewardsNotificationID id) {
  DCHECK(!id.empty());
  if (rewards_notifications_.find(id) == rewards_notifications_.end())
    return;
  RewardsNotification rewards_notification = rewards_notifications_[id];
  rewards_notifications_.erase(id);
  OnNotificationDeleted(rewards_notification);
}

void RewardsNotificationServiceImpl::DeleteAllNotifications() {
  rewards_notifications_.clear();
  OnAllNotificationsDeleted();
}

void RewardsNotificationServiceImpl::GetNotification(RewardsNotificationID id) {
  DCHECK(!id.empty());
  if (rewards_notifications_.find(id) == rewards_notifications_.end())
    return;
  OnGetNotification(rewards_notifications_[id]);
}

void RewardsNotificationServiceImpl::GetAllNotifications() {
  RewardsNotificationsList rewards_notifications_list;
  for (auto& item : rewards_notifications_) {
    rewards_notifications_list.push_back(item.second);
  }
  OnGetAllNotifications(rewards_notifications_list);
}

RewardsNotificationServiceImpl::RewardsNotificationID
RewardsNotificationServiceImpl::GenerateRewardsNotificationID() const {
  return base::StringPrintf(
      "%d", base::RandInt(0, std::numeric_limits<int32_t>::max()));
}

RewardsNotificationServiceImpl::RewardsNotificationTimestamp
RewardsNotificationServiceImpl::GenerateRewardsNotificationTimestamp() const {
  return base::Time::NowFromSystemTime().ToTimeT();
}

void RewardsNotificationServiceImpl::ReadRewardsNotifications() {
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

    if (notification_id.empty()) {
      int old_id;
      dict_value->GetInteger("id", &old_id);
      if (old_id == 0 && notification_type == 2)
        notification_id = "rewards_notification_grant";
      else
        notification_id = std::to_string(old_id);
    }

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

void RewardsNotificationServiceImpl::StoreRewardsNotifications() {
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

void RewardsNotificationServiceImpl::TriggerOnNotificationAdded(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnNotificationAdded(this, rewards_notification);
}

void RewardsNotificationServiceImpl::TriggerOnNotificationDeleted(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnNotificationDeleted(this, rewards_notification);
}

void RewardsNotificationServiceImpl::TriggerOnAllNotificationsDeleted() {
  for (auto& observer : observers_)
    observer.OnAllNotificationsDeleted(this);
}

void RewardsNotificationServiceImpl::TriggerOnGetNotification(
    const RewardsNotification& rewards_notification) {
  for (auto& observer : observers_)
    observer.OnGetNotification(this, rewards_notification);
}

void RewardsNotificationServiceImpl::TriggerOnGetAllNotifications(
    const RewardsNotificationsList& rewards_notifications_list) {
  for (auto& observer : observers_)
    observer.OnGetAllNotifications(this, rewards_notifications_list);
}

void RewardsNotificationServiceImpl::OnNotificationAdded(
    const RewardsNotification& rewards_notification) {
  TriggerOnNotificationAdded(rewards_notification);
}

void RewardsNotificationServiceImpl::OnNotificationDeleted(
    const RewardsNotification& rewards_notification) {
  TriggerOnNotificationDeleted(rewards_notification);
}

void RewardsNotificationServiceImpl::OnAllNotificationsDeleted() {
  TriggerOnAllNotificationsDeleted();
}

void RewardsNotificationServiceImpl::OnGetNotification(
    const RewardsNotification& rewards_notification) {
  TriggerOnGetNotification(rewards_notification);
}

void RewardsNotificationServiceImpl::OnGetAllNotifications(
    const RewardsNotificationsList& rewards_notifications_list) {
  TriggerOnGetAllNotifications(rewards_notifications_list);
}

}  // namespace brave_rewards
