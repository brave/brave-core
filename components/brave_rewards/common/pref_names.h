/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_

namespace brave_rewards::prefs {

// Used to enable/disable Rewards via a policy.
extern const char kDisabledByPolicy[];

// Stores the "Rewards version" in which the user's Rewards account was created.
// This value is intended to be used to distinguish "legacy" users when Rewards
// features are migrated. Note that if the user has not yet created a Rewards
// account, this value will default to the empty string.
extern const char kUserVersion[];
extern const char kCurrentUserVersion[];

extern const char kHideButton[];  // DEPRECATED
extern const char kShowButton[];  // DEPRECATED
extern const char kShowLocationBarButton[];
extern const char kEnabled[];  // DEPRECATED
extern const char kDeclaredGeo[];
extern const char kNotifications[];
extern const char kNotificationTimerInterval[];
extern const char kBackupNotificationInterval[];  // DEPRECATED
extern const char kBackupSucceeded[];             // DEPRECATED
extern const char kUserHasFunded[];               // DEPRECATED
extern const char kUserHasClaimedGrant[];         // DEPRECATED
extern const char kAddFundsNotification[];        // DEPRECATED
extern const char kNotificationStartupDelay[];
extern const char kExternalWallets[];  // DEPRECATED
extern const char kBadgeText[];
extern const char kUseRewardsStagingServer[];
extern const char kExternalWalletType[];

// Defined in core
extern const char kServerPublisherListStamp[];
extern const char kUpholdAnonAddress[];  // DEPRECATED
extern const char kPromotionLastFetchStamp[];
extern const char kPromotionCorruptedMigrated[];
extern const char kAnonTransferChecked[];
extern const char kVersion[];
extern const char kMinVisitTime[];
extern const char kMinVisits[];
extern const char kAllowNonVerified[];        // DEPRECATED
extern const char kAllowVideoContribution[];  // DEPRECATED
extern const char kScoreA[];
extern const char kScoreB[];
extern const char kAutoContributeEnabled[];
extern const char kAutoContributeAmount[];
extern const char kNextReconcileStamp[];
extern const char kCreationStamp[];
extern const char kRecoverySeed[];  // DEPRECATED
extern const char kPaymentId[];     // DEPRECATED
extern const char kInlineTipButtonsEnabled[];  // DEPRECATED
extern const char kInlineTipRedditEnabled[];   // DEPRECATED
extern const char kInlineTipTwitterEnabled[];  // DEPRECATED
extern const char kInlineTipGithubEnabled[];   // DEPRECATED
extern const char kParametersRate[];
extern const char kParametersAutoContributeChoice[];
extern const char kParametersAutoContributeChoices[];
extern const char kParametersTipChoices[];
extern const char kParametersMonthlyTipChoices[];
extern const char kParametersPayoutStatus[];
extern const char kParametersWalletProviderRegions[];
extern const char kParametersVBatDeadline[];
extern const char kParametersVBatExpired[];
extern const char kFetchOldBalance[];  // DEPRECATED
extern const char kEmptyBalanceChecked[];
extern const char kWalletBrave[];
extern const char kWalletUphold[];
extern const char kWalletBitflyer[];
extern const char kWalletGemini[];
extern const char kWalletZebPay[];
extern const char kWalletCreationEnvironment[];

// deprecated p3a prefs
extern const char kAdsWereDisabled[];
extern const char kHasAdsP3AState[];
extern const char kAdsEnabledTimeDelta[];
extern const char kAdsEnabledTimestamp[];

}  // namespace brave_rewards::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
