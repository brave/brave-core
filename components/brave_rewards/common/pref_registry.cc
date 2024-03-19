/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/pref_registry.h"

#include "brave/components/brave_rewards/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_rewards {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kShowLocationBarButton, true);
  registry->RegisterStringPref(prefs::kNotifications, "");
  registry->RegisterTimeDeltaPref(prefs::kNotificationTimerInterval,
                                  base::Days(1));
  registry->RegisterTimeDeltaPref(prefs::kBackupNotificationInterval,
                                  base::Days(30));
  registry->RegisterTimeDeltaPref(prefs::kNotificationStartupDelay,
                                  base::Seconds(30));
  registry->RegisterBooleanPref(prefs::kBackupSucceeded, false);
  registry->RegisterBooleanPref(prefs::kUserHasFunded, false);
  registry->RegisterBooleanPref(prefs::kUserHasClaimedGrant, false);
  registry->RegisterBooleanPref(prefs::kEnabled, false);
  registry->RegisterStringPref(prefs::kDeclaredGeo, "");
  registry->RegisterStringPref(prefs::kUserVersion, "");
  registry->RegisterDictionaryPref(prefs::kExternalWallets);
  registry->RegisterListPref(prefs::kP3APanelTriggerCount);
  registry->RegisterUint64Pref(prefs::kServerPublisherListStamp, 0ull);
  registry->RegisterStringPref(prefs::kUpholdAnonAddress, "");
  registry->RegisterStringPref(prefs::kBadgeText, "1");
  registry->RegisterBooleanPref(prefs::kUseRewardsStagingServer, false);
  registry->RegisterStringPref(prefs::kExternalWalletType, "");
  registry->RegisterDictionaryPref(prefs::kSelfCustodyAvailable);
  registry->RegisterBooleanPref(prefs::kSelfCustodyInviteDismissed, false);
  registry->RegisterBooleanPref(prefs::kAnonTransferChecked, false);
  registry->RegisterIntegerPref(prefs::kVersion, 0);
  registry->RegisterIntegerPref(prefs::kMinVisitTime, 8);
  registry->RegisterIntegerPref(prefs::kMinVisits, 1);
  registry->RegisterBooleanPref(prefs::kAllowNonVerified, true);
  registry->RegisterBooleanPref(prefs::kAllowVideoContribution, true);
  registry->RegisterDoublePref(prefs::kScoreA, 0.0);
  registry->RegisterDoublePref(prefs::kScoreB, 0.0);
  registry->RegisterBooleanPref(prefs::kAutoContributeEnabled, false);
  registry->RegisterDoublePref(prefs::kAutoContributeAmount, 0.0);
  registry->RegisterUint64Pref(prefs::kNextReconcileStamp, 0ull);
  registry->RegisterUint64Pref(prefs::kCreationStamp, 0ull);
  registry->RegisterStringPref(prefs::kRecoverySeed, "");
  registry->RegisterStringPref(prefs::kPaymentId, "");
  registry->RegisterBooleanPref(prefs::kInlineTipButtonsEnabled, false);
  registry->RegisterBooleanPref(prefs::kInlineTipRedditEnabled, false);
  registry->RegisterBooleanPref(prefs::kInlineTipTwitterEnabled, false);
  registry->RegisterBooleanPref(prefs::kInlineTipGithubEnabled, false);
  registry->RegisterDoublePref(prefs::kParametersRate, 0.0);
  registry->RegisterDoublePref(prefs::kParametersAutoContributeChoice, 0.0);
  registry->RegisterStringPref(prefs::kParametersAutoContributeChoices, "");
  registry->RegisterStringPref(prefs::kParametersTipChoices, "");
  registry->RegisterStringPref(prefs::kParametersMonthlyTipChoices, "");
  registry->RegisterStringPref(prefs::kParametersPayoutStatus, "");
  registry->RegisterDictionaryPref(prefs::kParametersWalletProviderRegions);
  registry->RegisterTimePref(prefs::kParametersVBatDeadline, base::Time());
  registry->RegisterBooleanPref(prefs::kParametersVBatExpired, false);
  registry->RegisterIntegerPref(prefs::kParametersTosVersion, 1);
  registry->RegisterStringPref(prefs::kWalletBrave, "");
  registry->RegisterStringPref(prefs::kWalletUphold, "");
  registry->RegisterStringPref(prefs::kWalletBitflyer, "");
  registry->RegisterStringPref(prefs::kWalletGemini, "");
  registry->RegisterStringPref(prefs::kWalletZebPay, "");
  registry->RegisterStringPref(prefs::kWalletSolana, "");
  registry->RegisterBooleanPref(prefs::kDisabledByPolicy, false);
  registry->RegisterIntegerPref(prefs::kWalletCreationEnvironment, -1);
  registry->RegisterIntegerPref(prefs::kTosVersion, 1);
  registry->RegisterListPref(prefs::kRewardsPageViewCount);
}

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 05/2023
  registry->RegisterBooleanPref(prefs::kAdsWereDisabled, false);
  registry->RegisterBooleanPref(prefs::kHasAdsP3AState, false);
  // Added 07/2023
  registry->RegisterTimePref(prefs::kAdsEnabledTimestamp, base::Time());
  registry->RegisterTimeDeltaPref(prefs::kAdsEnabledTimeDelta,
                                  base::TimeDelta());

  registry->RegisterUint64Pref(prefs::kPromotionLastFetchStamp, 0ull);
  registry->RegisterBooleanPref(prefs::kPromotionCorruptedMigrated, false);
  registry->RegisterBooleanPref(prefs::kFetchOldBalance, true);
  registry->RegisterBooleanPref(prefs::kEmptyBalanceChecked, false);
}

}  // namespace brave_rewards
