/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/reminder/reminder_manager.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/browser/reminder/reminder_info.h"
#include "brave/components/brave_ads/browser/reminder/reminder_util.h"
#include "url/gurl.h"

namespace brave_ads {

ReminderManager::ReminderManager(AdsService::Delegate& delegate)
    : delegate_(delegate) {}

ReminderManager::~ReminderManager() = default;

void ReminderManager::MaybeShow(mojom::ReminderType mojom_reminder_type) {
  ReminderInfo reminder = CreateReminder(mojom_reminder_type);

  delegate_->ShowNotificationAd(reminder.placement_id, reminder.title,
                                reminder.body);

  timeout_timers_.Start(
      reminder.placement_id,
      base::BindOnce(&ReminderManager::TimedOut, weak_factory_.GetWeakPtr(),
                     reminder.placement_id));

  shown_reminder_ = std::move(reminder);
}

bool ReminderManager::IsShowingReminder(const std::string& placement_id) const {
  return shown_reminder_ && shown_reminder_->placement_id == placement_id;
}

bool ReminderManager::MaybeHandleClosed(const std::string& placement_id) {
  if (!IsShowingReminder(placement_id)) {
    return false;
  }

  timeout_timers_.Stop(placement_id);
  shown_reminder_.reset();

  return true;
}

bool ReminderManager::MaybeHandleClicked(const std::string& placement_id) {
  if (!IsShowingReminder(placement_id)) {
    return false;
  }

  timeout_timers_.Stop(placement_id);

  delegate_->OpenNewTabWithUrl(GURL(shown_reminder_->target_url));
  delegate_->CloseNotificationAd(placement_id);

  // `shown_reminder_` stays set until `MaybeHandleClosed` observes the
  // resulting close event, otherwise it would be forwarded to ads core as a
  // real notification ad event.
  return true;
}

void ReminderManager::TimedOut(const std::string& placement_id) {
  timeout_timers_.Stop(placement_id);

  delegate_->CloseNotificationAd(placement_id);
}

void ReminderManager::Shutdown() {
  timeout_timers_.StopAll();
}

}  // namespace brave_ads
