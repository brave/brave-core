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
  registry->RegisterStringPref(prefs::kRewardsNotifications, "");
  registry->RegisterTimeDeltaPref(prefs::kRewardsNotificationTimerInterval,
                                  base::TimeDelta::FromDays(1));
  registry->RegisterTimeDeltaPref(prefs::kRewardsBackupNotificationInterval,
                                  base::TimeDelta::FromDays(30));
  registry->RegisterTimeDeltaPref(prefs::kRewardsNotificationStartupDelay,
                                  base::TimeDelta::FromSeconds(30));
  registry->RegisterBooleanPref(prefs::kRewardsBackupSucceeded, false);
  registry->RegisterBooleanPref(prefs::kRewardsUserHasFunded, false);
  registry->RegisterTimePref(prefs::kRewardsAddFundsNotification, base::Time());
  registry->RegisterBooleanPref(prefs::kBraveRewardsEnabled, false);
  registry->RegisterDictionaryPref(prefs::kRewardsExternalWallets);
  registry->RegisterUint64Pref(prefs::kStateServerPublisherListStamp, 0ull);
  registry->RegisterStringPref(prefs::kStateUpholdAnonAddress, "");
  registry->RegisterStringPref(prefs::kRewardsBadgeText, "1");
#if defined(OS_ANDROID)
  registry->RegisterBooleanPref(prefs::kUseRewardsStagingServer, false);
#endif
  registry->RegisterUint64Pref(prefs::kStatePromotionLastFetchStamp, 0ull);
  registry->RegisterBooleanPref(prefs::kStatePromotionCorruptedMigrated, false);
  registry->RegisterBooleanPref(prefs::kStateAnonTransferChecked, false);
  registry->RegisterIntegerPref(prefs::kStateVersion, 0);
  registry->RegisterIntegerPref(prefs::kStateMinVisitTime, 8);
  registry->RegisterIntegerPref(prefs::kStateMinVisits, 1);
  registry->RegisterBooleanPref(prefs::kStateAllowNonVerified, true);
  registry->RegisterBooleanPref(prefs::kStateAllowVideoContribution, true);
  registry->RegisterDoublePref(prefs::kStateScoreA, 0.0);
  registry->RegisterDoublePref(prefs::kStateScoreB, 0.0);
  registry->RegisterBooleanPref(prefs::kStateAutoContributeEnabled, false);
  registry->RegisterDoublePref(prefs::kStateAutoContributeAmount, 0.0);
  registry->RegisterUint64Pref(prefs::kStateNextReconcileStamp, 0ull);
  registry->RegisterUint64Pref(prefs::kStateCreationStamp, 0ull);
  registry->RegisterStringPref(prefs::kStateRecoverySeed, "");
  registry->RegisterStringPref(prefs::kStatePaymentId, "");
  registry->RegisterBooleanPref(prefs::kStateInlineTipRedditEnabled, false);
  registry->RegisterBooleanPref(prefs::kStateInlineTipTwitterEnabled, false);
  registry->RegisterBooleanPref(prefs::kStateInlineTipGithubEnabled, false);
  registry->RegisterDoublePref(prefs::kStateParametersRate, 0.0);
  registry->RegisterDoublePref(
      prefs::kStateParametersAutoContributeChoice,
      0.0);
  registry->RegisterStringPref(
      prefs::kStateParametersAutoContributeChoices,
      "");
  registry->RegisterStringPref(prefs::kStateParametersTipChoices, "");
  registry->RegisterStringPref(prefs::kStateParametersMonthlyTipChoices, "");
  registry->RegisterBooleanPref(prefs::kStateFetchOldBalance, true);
  registry->RegisterBooleanPref(prefs::kStateEmptyBalanceChecked, false);
}

}  // namespace brave_rewards
