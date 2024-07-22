/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder.h"

#include "base/location.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_feature.h"
#include "brave/components/brave_ads/core/internal/reminder/reminders/clicked_same_ad_multiple_times_reminder_util.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kMaybeShowReminderAfter = base::Milliseconds(100);

void MaybeShowReminder(const AdHistoryItemInfo& ad_history_item) {
  if (!base::FeatureList::IsEnabled(kReminderFeature)) {
    return;
  }

  if (DidUserClickTheSameAdMultipleTimes(ad_history_item)) {
    RemindUserTheyDoNotNeedToClickToEarnRewards();
  }
}

}  // namespace

Reminder::Reminder() {
  AdHistoryManager::GetInstance().AddObserver(this);
}

Reminder::~Reminder() {
  AdHistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reminder::MaybeShowReminderAfterDelay(
    const AdHistoryItemInfo& ad_history_item) {
  timer_.Start(FROM_HERE, kMaybeShowReminderAfter,
               base::BindOnce(&MaybeShowReminder, ad_history_item));
}

void Reminder::OnDidAppendAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
  MaybeShowReminderAfterDelay(ad_history_item);
}

}  // namespace brave_ads
