/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_

namespace brave_rewards {
namespace prefs {

extern const char kHideButton[];
extern const char kEnabled[];  // DEPRECATED
extern const char kNotifications[];
extern const char kNotificationTimerInterval[];
extern const char kBackupNotificationInterval[];
extern const char kBackupSucceeded[];
extern const char kUserHasFunded[];
extern const char kAddFundsNotification[];
extern const char kNotificationStartupDelay[];
extern const char kExternalWallets[];  // DEPRECATED
extern const char kBadgeText[];
extern const char kUseRewardsStagingServer[];
extern const char kOnboarded[];

// Defined in native-ledger
extern const char kServerPublisherListStamp[];
extern const char kUpholdAnonAddress[];  // DEPRECATED
extern const char kPromotionLastFetchStamp[];
extern const char kPromotionCorruptedMigrated[];
extern const char kAnonTransferChecked[];
extern const char kVersion[];
extern const char kMinVisitTime[];
extern const char kMinVisits[];
extern const char kAllowNonVerified[];
extern const char kAllowVideoContribution[];
extern const char kScoreA[];
extern const char kScoreB[];
extern const char kAutoContributeEnabled[];
extern const char kAutoContributeAmount[];
extern const char kNextReconcileStamp[];
extern const char kCreationStamp[];
extern const char kRecoverySeed[];  // DEPRECATED
extern const char kPaymentId[];   // DEPRECATED
extern const char kInlineTipRedditEnabled[];
extern const char kInlineTipTwitterEnabled[];
extern const char kInlineTipGithubEnabled[];
extern const char kParametersRate[];
extern const char kParametersAutoContributeChoice[];
extern const char kParametersAutoContributeChoices[];
extern const char kParametersTipChoices[];
extern const char kParametersMonthlyTipChoices[];
extern const char kFetchOldBalance[];
extern const char kEmptyBalanceChecked[];
extern const char kBAPReported[];
extern const char kWalletBrave[];
extern const char kWalletUphold[];
extern const char kWalletBitflyer[];

}  // namespace prefs
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
