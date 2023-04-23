/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder.h"

#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_features.h"
#include "brave/components/brave_ads/core/internal/reminder/reminders/clicked_same_ad_multiple_times_reminder_util.h"

namespace brave_ads {

namespace {

void MaybeShowReminder(const HistoryItemInfo& history_item) {
  if (!IsReminderEnabled()) {
    return;
  }

  if (DidUserClickTheSameAdMultipleTimes(history_item)) {
    RemindUserTheyDoNotNeedToClickToEarnRewards();
  }
}
}  // namespace

Reminder::Reminder() {
  HistoryManager::GetInstance().AddObserver(this);
}

Reminder::~Reminder() {
  HistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reminder::OnDidAddHistory(const HistoryItemInfo& history_item) {
  MaybeShowReminder(history_item);
}

}  // namespace brave_ads
