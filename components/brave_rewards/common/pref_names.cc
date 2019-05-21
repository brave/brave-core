/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_rewards {
namespace prefs {

const char kBraveRewardsEnabled[] = "brave.rewards.enabled";
const char kBraveRewardsEnabledMigrated[] = "brave.rewards.enabled_migrated";
const char kRewardsNotifications[] = "brave.rewards.notifications";
const char kRewardsNotificationTimerInterval[]=
    "brave.rewards.notification_timer_interval";
const char kRewardsBackupNotificationFrequency[] =
    "brave.rewards.backup_notification_frequency";
const char kRewardsBackupNotificationInterval[] =
    "brave.rewards.backup_notification_interval";
const char kRewardsBackupSucceeded[] = "brave.rewards.backup_succeeded";
const char kRewardsUserHasFunded[] = "brave.rewards.user_has_funded";
const char kRewardsAddFundsNotification[] =
    "brave.rewards.add_funds_notification";
const char kRewardsNotificationStartupDelay[] =
    "brave.rewards.notification_startup_delay";
const char kRewardsExternalWallets[] = "brave.rewards.external_wallets";
const char kStateServerPublisherListStamp[] =
    "brave.rewards.server_publisher_list_stamp";
const char kStateUpholdAnonAddress[] =
    "brave.rewards.uphold_anon_address";

const char kUseRewardsStagingServer[] = "brave.rewards.use_staging_server";
}  // namespace prefs
}  // namespace brave_rewards
