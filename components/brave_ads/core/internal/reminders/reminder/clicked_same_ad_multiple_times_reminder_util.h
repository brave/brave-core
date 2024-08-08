/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDER_CLICKED_SAME_AD_MULTIPLE_TIMES_REMINDER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDER_CLICKED_SAME_AD_MULTIPLE_TIMES_REMINDER_UTIL_H_

namespace brave_ads {

struct AdHistoryItemInfo;

bool DidUserClickTheSameAdMultipleTimes(
    const AdHistoryItemInfo& ad_history_item);

void RemindUserTheyDoNotNeedToClickToEarnRewards();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDER_CLICKED_SAME_AD_MULTIPLE_TIMES_REMINDER_UTIL_H_
