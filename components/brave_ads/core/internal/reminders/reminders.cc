/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminders.h"

#include "base/location.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/reminders/reminder/clicked_same_ad_multiple_times_reminder_util.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kMaybeShowReminderAfter = base::Milliseconds(100);

void MaybeShowReminder(const AdHistoryItemInfo& ad_history_item) {
  if (!base::FeatureList::IsEnabled(kRemindersFeature)) {
    return;
  }

  if (DidUserClickTheSameAdMultipleTimes(ad_history_item)) {
    RemindUserTheyDoNotNeedToClickToEarnRewards();
  }
}

}  // namespace

Reminders::Reminders() {
  AdHistoryManager::GetInstance().AddObserver(this);
}

Reminders::~Reminders() {
  AdHistoryManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Reminders::MaybeShowReminderAfterDelay(
    const AdHistoryItemInfo& ad_history_item) {
  timer_.Start(FROM_HERE, kMaybeShowReminderAfter,
               base::BindOnce(&MaybeShowReminder, ad_history_item));
}

void Reminders::OnDidAppendAdHistoryItem(
    const AdHistoryItemInfo& ad_history_item) {
  MaybeShowReminderAfterDelay(ad_history_item);
}

}  // namespace brave_ads
