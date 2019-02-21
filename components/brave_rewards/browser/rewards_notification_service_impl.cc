/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ledger/ledger_callback_handler.h"
#include "bat/ledger/publisher_info.h"
#include "brave/components/brave_rewards/common/pref_names.h"
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
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_rewards_notification_service_observer_ = 
          std::make_unique<ExtensionRewardsNotificationServiceObserver>(
              profile);
  AddObserver(extension_rewards_notification_service_observer_.get());
#endif
  ReadRewardsNotificationsJSON();
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
    RewardsNotificationID id,
    bool only_once) {
  DCHECK(type != REWARDS_NOTIFICATION_INVALID);
  if (id.empty()) {
    id = GenerateRewardsNotificationID();
  } else if (only_once) {
    if (std::find(
        rewards_notifications_displayed_.begin(),
        rewards_notifications_displayed_.end(),
        id) != rewards_notifications_displayed_.end()) {
      return;
    }
  }

  RewardsNotification rewards_notification(
      id, type, GenerateRewardsNotificationTimestamp(), std::move(args));
  rewards_notifications_[id] = rewards_notification;
  OnNotificationAdded(rewards_notification);

  if (only_once) {
    rewards_notifications_displayed_.push_back(id);
  }
}

void RewardsNotificationServiceImpl::DeleteNotification(RewardsNotificationID id) {
  DCHECK(!id.empty());
  RewardsNotification rewards_notification;
  if (rewards_notifications_.find(id) == rewards_notifications_.end()) {
    rewards_notification.id_ = id;
    rewards_notification.type_ = RewardsNotificationType::REWARDS_NOTIFICATION_INVALID;

    // clean up, so that we don't have long standing notifications
    if (rewards_notifications_.size() == 1) {
      rewards_notifications_.clear();
    }
  } else {
    rewards_notification = rewards_notifications_[id];
    rewards_notifications_.erase(id);
  }
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

void RewardsNotificationServiceImpl::GetNotifications() {
  RewardsNotificationsList rewards_notifications_list;
  for (auto& item : rewards_notifications_) {
    rewards_notifications_list.push_back(item.second);
  }
  OnGetAllNotifications(rewards_notifications_list);
}

const RewardsNotificationService::RewardsNotificationsMap&
RewardsNotificationServiceImpl::GetAllNotifications() const {
  return rewards_notifications_;
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

void RewardsNotificationServiceImpl::ReadRewardsNotificationsJSON() {
  std::string json = profile_->GetPrefs()->GetString(prefs::kRewardsNotifications);
  if (json.empty())
    return;
  std::unique_ptr<base::DictionaryValue> dictionary =
      base::DictionaryValue::From(base::JSONReader::Read(json));

  // legacy read
  if (!dictionary || !dictionary->is_dict()) {
    std::unique_ptr<base::ListValue> list =
      base::ListValue::From(base::JSONReader::Read(json));
    if (!list) {
      LOG(ERROR) << "Failed to deserialize rewards notifications on startup";
      return;
    }

    ReadRewardsNotifications(std::move(list));
    return;
  }

  base::ListValue* notifications;
  dictionary->GetList("notifications", &notifications);
  auto unique = std::make_unique<base::ListValue>(notifications->GetList());
  ReadRewardsNotifications(std::move(unique));

  base::ListValue* displayed;
  dictionary->GetList("displayed", &displayed);
  if (displayed && displayed->is_list()) {
    for (auto& it : *displayed) {
      rewards_notifications_displayed_.push_back(it.GetString());
    }
  }

}

void RewardsNotificationServiceImpl::ReadRewardsNotifications(
    std::unique_ptr<base::ListValue> root) {
  if (!root) {
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

    // The notification ID was originally an integer, but now it's a
    // string. For backwards compatibility, we need to handle the
    // case where the ID contains an invalid string or integer
    if (notification_id.empty()) {
      int old_id;
      dict_value->GetInteger("id", &old_id);
      if (old_id == 0 && notification_type == 2)
        notification_id = "rewards_notification_grant";
      else
        notification_id = std::to_string(old_id);
    } else if (notification_id == "0" && notification_type == 2) {
      notification_id = "rewards_notification_grant";
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
  base::DictionaryValue root;

  auto notifications = std::make_unique<base::ListValue>();
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
    notifications->Append(std::move(dict));
  }

  auto displayed = std::make_unique<base::ListValue>();
  for (auto& item : rewards_notifications_displayed_) {
    displayed->AppendString(item);
  }

  root.SetList("notifications", std::move(notifications));
  root.SetList("displayed", std::move(displayed));

  std::string result;
  if (!base::JSONWriter::Write(root, &result)) {
    LOG(ERROR) << "Failed to serialize rewards notifications on shutdown";
    return;
  }

  profile_->GetPrefs()->SetString(prefs::kRewardsNotifications, result);
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

std::string RewardsNotificationServiceImpl::GetGrantIdPrefix(
    const std::string& grant_type) {
  std::string prefix = grant_type == "ugp"
      ? "rewards_notification_grant_"
      : "rewards_notification_grant_ads_";
  return prefix;
}

void RewardsNotificationServiceImpl::OnGrant(RewardsService* rewards_service,
                                             unsigned int error_code,
                                             Grant properties) {
if (error_code == ledger::Result::LEDGER_OK) {
    std::string grant_type = properties.type;
    std::string prefix = GetGrantIdPrefix(grant_type);
    RewardsNotificationService::RewardsNotificationType notification_type
        = grant_type == "ugp"
        ? RewardsNotificationService::REWARDS_NOTIFICATION_GRANT
        : RewardsNotificationService::REWARDS_NOTIFICATION_GRANT_ADS;

    RewardsNotificationService::RewardsNotificationArgs args;
    AddNotification(notification_type,
                    args,
                    prefix + properties.promotionId,
                    true);
  }
}

void RewardsNotificationServiceImpl::OnGrantFinish(
    RewardsService* rewards_service,
    unsigned int result,
    Grant grant) {
  std::string grant_type = grant.type;
  std::string prefix = GetGrantIdPrefix(grant_type);

  DeleteNotification(prefix + grant.promotionId);
  // We keep it for back compatibility
  if (grant_type == "ugp") {
    DeleteNotification("rewards_notification_grant");
  }
}

void RewardsNotificationServiceImpl::OnReconcileComplete(
    RewardsService* rewards_service,
    unsigned int result,
    const std::string& viewing_id,
    const std::string& category,
    const std::string& probi) {
  if ((result == ledger::Result::LEDGER_OK &&
       category ==
          std::to_string(ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE)) ||
       result == ledger::Result::LEDGER_ERROR ||
       result == ledger::Result::NOT_ENOUGH_FUNDS ||
       result == ledger::Result::TIP_ERROR) {
    RewardsNotificationService::RewardsNotificationArgs args;
    args.push_back(viewing_id);
    args.push_back(std::to_string(result));
    args.push_back(category);
    args.push_back(probi);

    AddNotification(
        RewardsNotificationService::REWARDS_NOTIFICATION_AUTO_CONTRIBUTE,
        args,
        "contribution_" + viewing_id);
  }
}

}  // namespace brave_rewards
