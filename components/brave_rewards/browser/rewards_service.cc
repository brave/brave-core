/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "components/prefs/pref_registry_simple.h"

#if !BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "content/public/common/referrer.h"
#endif

namespace brave_rewards {

#if !BUILDFLAG(BRAVE_REWARDS_ENABLED)
bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const content::Referrer& referrer) {
  return false;
}
#endif

RewardsService::RewardsService() {
}

RewardsService::~RewardsService() {
}

void RewardsService::AddObserver(RewardsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void RewardsService::RemoveObserver(RewardsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// static
void RewardsService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kNotifications, "");
  registry->RegisterTimeDeltaPref(prefs::kNotificationTimerInterval,
                                  base::TimeDelta::FromDays(1));
  registry->RegisterTimeDeltaPref(prefs::kBackupNotificationInterval,
                                  base::TimeDelta::FromDays(30));
  registry->RegisterTimeDeltaPref(prefs::kNotificationStartupDelay,
                                  base::TimeDelta::FromSeconds(30));
  registry->RegisterBooleanPref(prefs::kBackupSucceeded, false);
  registry->RegisterBooleanPref(prefs::kUserHasFunded, false);
  registry->RegisterTimePref(prefs::kAddFundsNotification, base::Time());
  registry->RegisterBooleanPref(prefs::kEnabled, false);
  registry->RegisterDictionaryPref(prefs::kExternalWallets);
  registry->RegisterUint64Pref(prefs::kServerPublisherListStamp, 0ull);
  registry->RegisterStringPref(prefs::kUpholdAnonAddress, "");
  registry->RegisterStringPref(prefs::kBadgeText, "1");
#if defined(OS_ANDROID)
  registry->RegisterBooleanPref(prefs::kUseRewardsStagingServer, false);
#endif
  registry->RegisterTimePref(prefs::kOnboarded, base::Time());
  registry->RegisterUint64Pref(prefs::kPromotionLastFetchStamp, 0ull);
  registry->RegisterBooleanPref(prefs::kPromotionCorruptedMigrated, false);
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
  registry->RegisterBooleanPref(prefs::kInlineTipRedditEnabled, true);
  registry->RegisterBooleanPref(prefs::kInlineTipTwitterEnabled, true);
  registry->RegisterBooleanPref(prefs::kInlineTipGithubEnabled, true);
  registry->RegisterDoublePref(prefs::kParametersRate, 0.0);
  registry->RegisterDoublePref(
      prefs::kParametersAutoContributeChoice,
      0.0);
  registry->RegisterStringPref(
      prefs::kParametersAutoContributeChoices,
      "");
  registry->RegisterStringPref(prefs::kParametersTipChoices, "");
  registry->RegisterStringPref(prefs::kParametersMonthlyTipChoices, "");
  registry->RegisterBooleanPref(prefs::kFetchOldBalance, true);
  registry->RegisterBooleanPref(prefs::kEmptyBalanceChecked, false);
  registry->RegisterBooleanPref(prefs::kBAPReported, false);
  registry->RegisterStringPref(prefs::kWalletBrave, "");
  registry->RegisterStringPref(prefs::kWalletUphold, "");
  registry->RegisterStringPref(prefs::kWalletBitflyer, "");
}

}  // namespace brave_rewards
