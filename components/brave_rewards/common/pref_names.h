/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_

namespace brave_rewards {
namespace prefs {

extern const char kHideBraveRewardsButton[];
extern const char kBraveRewardsEnabled[];
extern const char kRewardsNotifications[];
extern const char kRewardsNotificationTimerInterval[];
extern const char kRewardsBackupNotificationInterval[];
extern const char kRewardsBackupSucceeded[];
extern const char kRewardsUserHasFunded[];
extern const char kRewardsAddFundsNotification[];
extern const char kRewardsNotificationStartupDelay[];
extern const char kRewardsExternalWallets[];
extern const char kRewardsBadgeText[];

// Defined in native-ledger
extern const char kStateServerPublisherListStamp[];
extern const char kStateUpholdAnonAddress[];  // DEPRECATED
extern const char kStatePromotionLastFetchStamp[];
extern const char kStatePromotionCorruptedMigrated[];
extern const char kStateAnonTransferChecked[];
extern const char kStateVersion[];
extern const char kStateMinVisitTime[];
extern const char kStateMinVisits[];
extern const char kStateAllowNonVerified[];
extern const char kStateAllowVideoContribution[];
extern const char kStateScoreA[];
extern const char kStateScoreB[];
extern const char kStateAutoContributeEnabled[];
extern const char kStateAutoContributeAmount[];
extern const char kStateNextReconcileStamp[];
extern const char kStateCreationStamp[];
extern const char kStateRecoverySeed[];
extern const char kStatePaymentId[];
extern const char kStateInlineTipRedditEnabled[];
extern const char kStateInlineTipTwitterEnabled[];
extern const char kStateInlineTipGithubEnabled[];
extern const char kStateParametersRate[];
extern const char kStateParametersAutoContributeChoice[];
extern const char kStateParametersAutoContributeChoices[];
extern const char kStateParametersTipChoices[];
extern const char kStateParametersMonthlyTipChoices[];
extern const char kStateFetchOldBalance[];
extern const char kStateEmptyBalanceChecked[];

extern const char kUseRewardsStagingServer[];
}  // namespace prefs
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
