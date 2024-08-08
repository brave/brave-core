/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminder/clicked_same_ad_multiple_times_reminder_util.h"

#include <cstddef>

#include "base/check_op.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

bool ShouldRemindUser() {
  return base::FeatureList::IsEnabled(kRemindersFeature) &&
         kRemindUserIfClickingTheSameAdAfter.Get() > 0;
}

bool DidUserClickTheSameAdMultipleTimes(const std::string& creative_instance_id,
                                        const AdHistoryList& ad_history) {
  const size_t count = base::ranges::count_if(
      ad_history,
      [&creative_instance_id](const AdHistoryItemInfo& ad_history_item) {
        return ad_history_item.creative_instance_id == creative_instance_id &&
               ad_history_item.confirmation_type == ConfirmationType::kClicked;
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
  AdsNotifierManager::GetInstance().NotifyRemindUser(
      mojom::ReminderType::kClickedSameAdMultipleTimes);
}

}  // namespace brave_ads
