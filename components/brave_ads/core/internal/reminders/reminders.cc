/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminders.h"

#include "base/location.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/reminders/reminder/clicked_same_ad_multiple_times_reminder_util.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_constants.h"

namespace brave_ads {

Reminders::Reminders() {
  AdHistoryManager::GetInstance().AddObserver(this);
}

Reminders::~Reminders() {
  AdHistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reminders::MaybeShowReminders(const AdHistoryItemInfo& ad_history_item) {
  if (!ShouldRemindUser()) {
    return;
  }

  MaybeShowUserClickTheSameAdMultipleTimesReminderAfterDelay(ad_history_item);
}

bool Reminders::CanShowUserClickTheSameAdMultipleTimesReminder(
    const AdHistoryItemInfo& ad_history_item) {
  return !PlatformHelper::GetInstance().IsMobile() &&
         ad_history_item.type == mojom::AdType::kNotificationAd &&
         ad_history_item.confirmation_type == mojom::ConfirmationType::kClicked;
}

void Reminders::MaybeShowUserClickTheSameAdMultipleTimesReminderAfterDelay(
    const AdHistoryItemInfo& ad_history_item) {
  if (!CanShowUserClickTheSameAdMultipleTimesReminder(ad_history_item)) {
    return;
  }

  // The user clicked on a notification ad, so we should delay showing the
  // reminder to ensure the notification ad is removed from the screen.
  timer_.Start(FROM_HERE, kMaybeShowReminderAfter,
               base::BindOnce(
                   &Reminders::MaybeShowUserClickTheSameAdMultipleTimesReminder,
                   weak_factory_.GetWeakPtr(), ad_history_item));
}

void Reminders::MaybeShowUserClickTheSameAdMultipleTimesReminder(
    const AdHistoryItemInfo& ad_history_item) {
  database::table::AdHistory database_table;
  database_table.GetForCreativeInstanceId(
      ad_history_item.creative_instance_id,
      base::BindOnce(
          &Reminders::MaybeShowUserClickTheSameAdMultipleTimesReminderCallback,
          weak_factory_.GetWeakPtr(), ad_history_item.creative_instance_id));
}

void Reminders::MaybeShowUserClickTheSameAdMultipleTimesReminderCallback(
    const std::string& creative_instance_id,
    std::optional<AdHistoryList> ad_history) {
  if (!ad_history) {
    return;
  }

  if (DidUserClickTheSameAdMultipleTimes(creative_instance_id, *ad_history)) {
    RemindUserTheyDoNotNeedToClickToEarnRewards();
  }
}

void Reminders::OnDidAddAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
  MaybeShowReminders(ad_history_item);
}

}  // namespace brave_ads
