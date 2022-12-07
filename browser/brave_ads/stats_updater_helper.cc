/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/stats_updater_helper.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

StatsUpdaterHelper::StatsUpdaterHelper()
    : local_state_(g_browser_process->local_state()),
      profile_manager_(g_browser_process->profile_manager()) {
  last_used_profile_pref_change_registrar_.Init(local_state_);
  last_used_profile_pref_change_registrar_.Add(
      ::prefs::kProfileLastUsed,
      base::BindRepeating(&StatsUpdaterHelper::OnLastUsedProfileChanged,
                          base::Unretained(this)));

  profile_manager_observer_.Observe(profile_manager_);
}

StatsUpdaterHelper::~StatsUpdaterHelper() = default;

void StatsUpdaterHelper::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(ads::prefs::kEnabledForLastProfile, false);
}

void StatsUpdaterHelper::OnProfileAdded(Profile* profile) {
  if (profile->GetBaseName() ==
      local_state_->GetFilePath(::prefs::kProfileLastUsed)) {
    OnLastUsedProfileChanged();
  }
}

void StatsUpdaterHelper::OnProfileManagerDestroying() {
  ads_enabled_pref_change_registrar_.RemoveAll();
  profile_manager_observer_.Reset();
}

void StatsUpdaterHelper::OnLastUsedProfileChanged() {
  base::FilePath last_used_profile_path =
      local_state_->GetFilePath(::prefs::kProfileLastUsed);
  Profile* profile = profile_manager_->GetProfileByPath(
      profile_manager_->user_data_dir().Append(last_used_profile_path));
  if (profile == nullptr) {
    return;
  }
  profile_prefs_ = profile->GetPrefs();
  ads_enabled_pref_change_registrar_.RemoveAll();
  ads_enabled_pref_change_registrar_.Init(profile_prefs_);
  ads_enabled_pref_change_registrar_.Add(
      ads::prefs::kEnabled,
      base::BindRepeating(&StatsUpdaterHelper::UpdateLocalStateAdsEnabled,
                          base::Unretained(this)));
  UpdateLocalStateAdsEnabled();
}

void StatsUpdaterHelper::UpdateLocalStateAdsEnabled() {
  if (profile_prefs_ == nullptr) {
    return;
  }
  // Copying enabled pref to local state so the stats updater does not depend on
  // the profile
  local_state_->SetBoolean(ads::prefs::kEnabledForLastProfile,
                           profile_prefs_->GetBoolean(ads::prefs::kEnabled));
}

}  // namespace brave_ads
