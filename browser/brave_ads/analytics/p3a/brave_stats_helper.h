/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ANALYTICS_P3A_BRAVE_STATS_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_ANALYTICS_P3A_BRAVE_STATS_HELPER_H_

#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;
class ProfileManager;
class Profile;

namespace brave_ads {
inline constexpr char kAdsEnabledInstallationTimeHistogramName[] =
    "Brave.Rewards.EnabledInstallationTime";

class BraveStatsHelper : public ProfileManagerObserver, public ProfileObserver {
 public:
  BraveStatsHelper(PrefService* local_state, ProfileManager* profile_manager);
  ~BraveStatsHelper() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void OnProfileAdded(Profile* profile) override;
  void OnProfileManagerDestroying() override;

  void OnProfileWillBeDestroyed(Profile* profile) override;

  void SetFirstRunTimeForTesting(base::Time time);

 private:
  PrefService* GetLastUsedProfilePrefs();
  void OnLastUsedProfileChanged();

  void Update();
  void UpdateLocalStateAdsEnabled(bool is_enabled_for_current_profile);
  void MaybeReportAdsInstallationTimeMetric(
      bool is_enabled_for_current_profile);

#if !BUILDFLAG(IS_ANDROID)
  PrefChangeRegistrar last_used_profile_pref_change_registrar_;
#endif
  PrefChangeRegistrar ads_enabled_pref_change_registrar_;
  base::ScopedObservation<Profile, ProfileObserver>
      current_profile_observation_{this};

  base::ScopedObservation<ProfileManager, ProfileManagerObserver>
      profile_manager_observer_{this};

  raw_ptr<PrefService> local_state_ = nullptr;         // Not owned.
  raw_ptr<ProfileManager> profile_manager_ = nullptr;  // Not owned.

  base::Time testing_first_run_time_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ANALYTICS_P3A_BRAVE_STATS_HELPER_H_
