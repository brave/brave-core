/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder_features.h"

#include "base/metrics/field_trial_params.h"

namespace brave_ads::features {

namespace {

constexpr char kRemindUserIfClickingTheSameAdAfterTrialParamName[] =
    "remind_user_if_clicking_the_same_ad_after";
constexpr int kRemindUserIfClickingTheSameAdAfterDefaultValue = 3;

}  // namespace

BASE_FEATURE(kReminder, "Reminder", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kReminder);
}

size_t GetRemindUserIfClickingTheSameAdAfter() {
  return static_cast<size_t>(GetFieldTrialParamByFeatureAsInt(
      kReminder, kRemindUserIfClickingTheSameAdAfterTrialParamName,
      kRemindUserIfClickingTheSameAdAfterDefaultValue));
}

}  // namespace brave_ads::features
