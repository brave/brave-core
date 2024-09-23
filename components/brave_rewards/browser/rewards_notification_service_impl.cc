/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"

#include <limits>
#include <optional>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

namespace brave_rewards {

RewardsNotificationServiceImpl::RewardsNotificationServiceImpl(
    PrefService* prefs)
    : prefs_(prefs) {
  ReadRewardsNotificationsJSON();
}

RewardsNotificationServiceImpl::~RewardsNotificationServiceImpl() {
  StoreRewardsNotifications();
  if (extension_observer_) {
    RemoveObserver(extension_observer_.get());
  }
}

void RewardsNotificationServiceImpl::Init(
    std::unique_ptr<RewardsNotificationServiceObserver> extension_observer) {
  if (extension_observer) {
    extension_observer_ = std::move(extension_observer);
    AddObserver(extension_observer_.get());
  }
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
    if (base::Contains(rewards_notifications_displayed_, id)) {
      return;
    }
  }

  RewardsNotification rewards_notification(
      id, type, GenerateRewardsNotificationTimestamp(), std::move(args));
  rewards_notifications_[id] = rewards_notification;
  StoreRewardsNotifications();
  OnNotificationAdded(rewards_notification);

  if (only_once) {
    rewards_notifications_displayed_.push_back(id);
  }
}

void RewardsNotificationServiceImpl::DeleteNotification(
    RewardsNotificationID id) {
  DCHECK(!id.empty());
  RewardsNotification rewards_notification;
  if (rewards_notifications_.find(id) == rewards_notifications_.end()) {
    rewards_notification.id_ = id;
    rewards_notification.type_ =
        RewardsNotificationType::REWARDS_NOTIFICATION_INVALID;

    // clean up, so that we don't have long standing notifications
    if (rewards_notifications_.size() == 1) {
      rewards_notifications_.clear();
    }
  } else {
    rewards_notification = rewards_notifications_[id];
    rewards_notifications_.erase(id);
  }
  StoreRewardsNotifications();

  OnNotificationDeleted(rewards_notification);
}

void RewardsNotificationServiceImpl::DeleteAllNotifications(
    const bool delete_displayed) {
  bool displayed = delete_displayed;

#if BUILDFLAG(IS_ANDROID)
  displayed = true;
#endif

  if (displayed) {
    rewards_notifications_displayed_.clear();
  }

  rewards_notifications_.clear();
  StoreRewardsNotifications();
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
  std::string json = prefs_->GetString(prefs::kNotifications);
  if (json.empty())
    return;
  std::optional<base::Value> parsed = base::JSONReader::Read(json);

  // legacy read
  if (!parsed || (!parsed->is_dict() && !parsed->is_list())) {
    LOG(ERROR) << "Failed to deserialize rewards notifications on startup";
    return;
  }
  if (parsed->is_list()) {
    ReadRewardsNotifications(parsed->GetList());
    return;
  }

  const base::Value::Dict& dict = parsed->GetDict();

  const base::Value::List* notifications = dict.FindList("notifications");
  if (notifications) {
    ReadRewardsNotifications(*notifications);
  }

  const base::Value::List* displayed = dict.FindList("displayed");
  if (displayed) {
    for (const auto& item : *displayed) {
      DCHECK(item.is_string());
      rewards_notifications_displayed_.push_back(item.GetString());
    }
  }
}

void RewardsNotificationServiceImpl::ReadRewardsNotifications(
    const base::Value::List& root) {
  for (const auto& item : root) {
    if (!item.is_dict())
      continue;
    const base::Value::Dict& dict = item.GetDict();
    std::string notification_id;
    const std::string* notification_id_opt = dict.FindString("id");
    if (notification_id_opt)
      notification_id = *notification_id_opt;
    int notification_type = dict.FindInt("type").value_or(0);
    int notification_timestamp = dict.FindDouble("timestamp").value_or(0.0);
    RewardsNotificationArgs notification_args;

    // The notification ID was originally an integer, but now it's a
    // string. For backwards compatibility, we need to handle the
    // case where the ID contains an invalid string or integer
    if (notification_id.empty()) {
      int old_id = dict.FindInt("id").value_or(0);
      if (old_id == 0 && notification_type == 2)
        notification_id = "rewards_notification_grant";
      else
        notification_id = base::NumberToString(old_id);
    } else if (notification_id == "0" && notification_type == 2) {
      notification_id = "rewards_notification_grant";
    }

    const base::Value::List* args = dict.FindList("args");
    if (args) {
      for (auto& arg : *args) {
        std::string arg_string = arg.GetString();
        notification_args.push_back(arg_string);
      }
    }

    RewardsNotification notification(notification_id,
        static_cast<RewardsNotificationType>(notification_type),
        notification_timestamp,
        notification_args);
    rewards_notifications_[notification.id_] = notification;
  }
}

void RewardsNotificationServiceImpl::StoreRewardsNotifications() {
  base::Value::Dict root;

  base::Value::List notifications;
  for (auto& item : rewards_notifications_) {
    base::Value::Dict dict;
    dict.Set("id", item.second.id_);
    dict.Set("type", item.second.type_);
    dict.Set("timestamp", static_cast<double>(item.second.timestamp_));
    base::Value::List args;
    for (auto& arg : item.second.args_) {
      args.Append(arg);
    }
    dict.Set("args", std::move(args));
    notifications.Append(std::move(dict));
  }

  base::Value::List displayed;
  for (auto& item : rewards_notifications_displayed_) {
    displayed.Append(item);
  }

  root.Set("notifications", std::move(notifications));
  root.Set("displayed", std::move(displayed));

  std::string result;
  if (!base::JSONWriter::Write(root, &result)) {
    LOG(ERROR) << "Failed to serialize rewards notifications on shutdown";
    return;
  }

  prefs_->SetString(prefs::kNotifications, result);
}

bool RewardsNotificationServiceImpl::Exists(RewardsNotificationID id) const {
  DCHECK(!id.empty());
  if (rewards_notifications_.find(id) == rewards_notifications_.end()) {
    return false;
  }

  return true;
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

void RewardsNotificationServiceImpl::OnReconcileComplete(
    RewardsService* rewards_service,
    const mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const mojom::RewardsType type,
    const mojom::ContributionProcessor processor) {
  if (type == mojom::RewardsType::ONE_TIME_TIP) {
    return;
  }

  const bool completed_auto_contribute =
      result == mojom::Result::OK &&
      type == mojom::RewardsType::AUTO_CONTRIBUTE;

  if (completed_auto_contribute || result == mojom::Result::NOT_ENOUGH_FUNDS ||
      result == mojom::Result::FAILED || result == mojom::Result::TIP_ERROR) {
    RewardsNotificationService::RewardsNotificationArgs args;
    args.push_back(contribution_id);
    args.push_back(base::NumberToString(static_cast<int>(result)));
    args.push_back(base::NumberToString(static_cast<int>(type)));
    args.push_back(base::NumberToString(amount));

    AddNotification(
        RewardsNotificationService::REWARDS_NOTIFICATION_AUTO_CONTRIBUTE,
        args,
        "contribution_" + contribution_id);
  }
}

}  // namespace brave_rewards
