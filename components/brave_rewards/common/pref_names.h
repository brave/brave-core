/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_

namespace brave_rewards {
namespace prefs {

extern const char kBraveRewardsEnabled[];
extern const char kBraveRewardsEnabledMigrated[];
extern const char kRewardsNotifications[];
extern const char kRewardsNotificationTimerInterval[];
extern const char kRewardsBackupNotificationFrequency[];
extern const char kRewardsBackupNotificationInterval[];
extern const char kRewardsBackupSucceeded[];
extern const char kRewardsUserHasFunded[];
extern const char kRewardsAddFundsNotification[];
extern const char kRewardsNotificationStartupDelay[];
extern const char kRewardsExternalWallets[];

// Defined in native-ledger
extern const char kStateServerPublisherListStamp[];
extern const char kStateUpholdAnonAddress[];

extern const char kUseRewardsStagingServer[];
}  // namespace prefs
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
