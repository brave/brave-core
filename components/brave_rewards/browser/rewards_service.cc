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
  registry->RegisterTimeDeltaPref(prefs::kRewardsBackupNotificationFrequency,
                                  base::TimeDelta::FromDays(7));
  registry->RegisterTimeDeltaPref(prefs::kRewardsBackupNotificationInterval,
                                  base::TimeDelta::FromDays(7));
  registry->RegisterTimeDeltaPref(prefs::kRewardsNotificationStartupDelay,
                                  base::TimeDelta::FromSeconds(30));
  registry->RegisterBooleanPref(prefs::kRewardsBackupSucceeded, false);
  registry->RegisterBooleanPref(prefs::kRewardsUserHasFunded, false);
  registry->RegisterTimePref(prefs::kRewardsAddFundsNotification, base::Time());
  registry->RegisterBooleanPref(prefs::kBraveRewardsEnabled, false);
  registry->RegisterBooleanPref(prefs::kBraveRewardsEnabledMigrated, false);
  registry->RegisterDictionaryPref(prefs::kRewardsExternalWallets);
  registry->RegisterUint64Pref(prefs::kStateServerPublisherListStamp, 0ull);
}

}  // namespace brave_rewards
