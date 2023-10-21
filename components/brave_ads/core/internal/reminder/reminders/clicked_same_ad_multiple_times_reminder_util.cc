/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminders/clicked_same_ad_multiple_times_reminder_util.h"

#include "base/check_op.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/reminder/reminder_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"

namespace brave_ads {

namespace {

bool CanRemind(const HistoryItemInfo& history_item) {
  return !PlatformHelper::GetInstance().IsMobile() &&
         kRemindUserIfClickingTheSameAdAfter.Get() > 0 &&
         history_item.ad_content.type == AdType::kNotificationAd &&
         history_item.ad_content.confirmation_type ==
             ConfirmationType::kClicked;
}

}  // namespace

bool DidUserClickTheSameAdMultipleTimes(const HistoryItemInfo& history_item) {
  if (!CanRemind(history_item)) {
    return false;
  }

  const size_t count = base::ranges::count_if(
      HistoryManager::Get(), [&history_item](const HistoryItemInfo& other) {
        return other.ad_content.confirmation_type ==
                   ConfirmationType::kClicked &&
               other.ad_content.creative_instance_id ==
                   history_item.ad_content.creative_instance_id;
      });

  if (count == 0) {
    return false;
  }

  const size_t remind_user_if_clicking_the_same_ad_after =
      kRemindUserIfClickingTheSameAdAfter.Get();
  CHECK_GT(remind_user_if_clicking_the_same_ad_after, 0U);

  return (count - 1) % remind_user_if_clicking_the_same_ad_after ==
         remind_user_if_clicking_the_same_ad_after - 1;
}

void RemindUserTheyDoNotNeedToClickToEarnRewards() {
  ShowReminder(mojom::ReminderType::kClickedSameAdMultipleTimes);
}

}  // namespace brave_ads
