/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BRAVE_STATS_UPDATER_HELPER_H_
#define BRAVE_BROWSER_BRAVE_ADS_BRAVE_STATS_UPDATER_HELPER_H_

#include "base/scoped_observation.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class Profile;

namespace brave_ads {

class BraveStatsUpdaterHelper : public ProfileManagerObserver {
 public:
  BraveStatsUpdaterHelper();
  ~BraveStatsUpdaterHelper() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void OnProfileAdded(Profile* profile) override;
  void OnProfileManagerDestroying() override;

 private:
  PrefService* GetLastUsedProfilePrefs();
  void OnLastUsedProfileChanged();
  void UpdateLocalStateAdsEnabled();

#if !BUILDFLAG(IS_ANDROID)
  PrefChangeRegistrar last_used_profile_pref_change_registrar_;
#endif
  PrefChangeRegistrar ads_enabled_pref_change_registrar_;

  base::ScopedObservation<ProfileManager, ProfileManagerObserver>
      profile_manager_observer_{this};

  raw_ptr<PrefService> local_state_;
  raw_ptr<ProfileManager> profile_manager_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BRAVE_STATS_UPDATER_HELPER_H_
