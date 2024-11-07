/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/day_zero_browser_ui_expt/day_zero_browser_ui_expt_manager.h"

#include <optional>

#include "base/functional/bind.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/browser/day_zero_browser_ui_expt/pref_names.h"
#include "brave/components/brave_news/common/locales_helper.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/DayZeroHelper_jni.h"
#endif  // #BUILDFLAG(IS_ANDROID)

// static
void DayZeroBrowserUIExptManager::RegisterLocalStatePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kDayZeroExperimentTargetInstall, false);
}

// static
std::unique_ptr<DayZeroBrowserUIExptManager>
DayZeroBrowserUIExptManager::Create(ProfileManager* profile_manager) {
  if (!base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)) {
    return nullptr;
  }

  std::optional<std::string> day_zero_variant;
  if (base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)) {
    day_zero_variant = features::kBraveDayZeroExperimentVariant.Get();
  }

  CHECK(g_browser_process && g_browser_process->local_state());
  auto* local_state = g_browser_process->local_state();
  if (!day_zero_variant) {
    VLOG(2) << __func__ << ": Day zero Expt variant is not available";
    local_state->SetBoolean(kDayZeroExperimentTargetInstall, false);
    return nullptr;
  }

  if (day_zero_variant != "a") {
    VLOG(2) << __func__ << ": Day zero Expt variant is not 'a'";
    local_state->SetBoolean(kDayZeroExperimentTargetInstall, false);
    return nullptr;
  }

  if (brave_stats::IsFirstRun(local_state)) {
    VLOG(2) << __func__ << ": Set Day zero experiment to this fresh user.";
    local_state->SetBoolean(kDayZeroExperimentTargetInstall, true);
  }

  if (!local_state->GetBoolean(kDayZeroExperimentTargetInstall)) {
    VLOG(2) << __func__
            << ": This is existing user. Day zero experiment is only applied "
               "to fresh user.";
    return nullptr;
  }

  // base::WrapUnique for using private ctor.
  return base::WrapUnique(
      new DayZeroBrowserUIExptManager(profile_manager, local_state));
}

DayZeroBrowserUIExptManager::DayZeroBrowserUIExptManager(
    ProfileManager* profile_manager,
    PrefService* local_state)
    : profile_manager_(*profile_manager) {
  p3a_enabled_.Init(
      p3a::kP3AEnabled, local_state,
      base::BindRepeating(&DayZeroBrowserUIExptManager::OnP3AEnabledChanged,
                          base::Unretained(this)));

  if (IsP3AEnabled()) {
    SetDayZeroBrowserUIForAllProfiles();
  }

  observation_.Observe(&(*profile_manager_));
}

DayZeroBrowserUIExptManager::~DayZeroBrowserUIExptManager() {
  if (observation_.IsObserving()) {
    observation_.Reset();
  }
}

void DayZeroBrowserUIExptManager::OnProfileAdded(Profile* profile) {
  if (!IsP3AEnabled()) {
    return;
  }

  SetForDayZeroBrowserUI(profile);
}

void DayZeroBrowserUIExptManager::OnProfileManagerDestroying() {
  if (observation_.IsObserving()) {
    observation_.Reset();
  }
}

void DayZeroBrowserUIExptManager::SetForDayZeroBrowserUI(Profile* profile) {
  VLOG(2) << __func__ << " Update prefs for day zero expt.";

  auto* prefs = profile->GetPrefs();
  prefs->SetDefaultPrefValue(kNewTabPageShowRewards, base::Value(false));
  prefs->SetDefaultPrefValue(kNewTabPageShowBraveTalk, base::Value(false));
  prefs->SetDefaultPrefValue(kShowWalletIconOnToolbar, base::Value(false));
  prefs->SetDefaultPrefValue(brave_rewards::prefs::kShowLocationBarButton,
                             base::Value(false));
  bool should_show_ntp_si_and_news = false;
#if BUILDFLAG(IS_ANDROID)
  should_show_ntp_si_and_news = true;
  Java_DayZeroHelper_setDayZeroExptAndroid(
      base::android::AttachCurrentThread(), false);
#endif  // #BUILDFLAG(IS_ANDROID)
  prefs->SetDefaultPrefValue(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             base::Value(should_show_ntp_si_and_news));
  prefs->SetDefaultPrefValue(brave_news::prefs::kNewTabPageShowToday,
                             base::Value(should_show_ntp_si_and_news));
}

void DayZeroBrowserUIExptManager::ResetForDayZeroBrowserUI(Profile* profile) {
  VLOG(2) << __func__ << " Update prefs for day zero expt.";

  auto* prefs = profile->GetPrefs();
  prefs->SetDefaultPrefValue(kNewTabPageShowRewards, base::Value(true));
  prefs->SetDefaultPrefValue(kNewTabPageShowBraveTalk, base::Value(true));
  prefs->SetDefaultPrefValue(kShowWalletIconOnToolbar, base::Value(true));
  prefs->SetDefaultPrefValue(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             base::Value(true));
  prefs->SetDefaultPrefValue(brave_rewards::prefs::kShowLocationBarButton,
                             base::Value(true));
  prefs->SetDefaultPrefValue(
      brave_news::prefs::kNewTabPageShowToday,
      base::Value(brave_news::IsUserInDefaultEnabledLocale()));
#if BUILDFLAG(IS_ANDROID)
  Java_DayZeroHelper_setDayZeroExptAndroid(
      base::android::AttachCurrentThread(), true);
#endif  // #BUILDFLAG(IS_ANDROID)
}

void DayZeroBrowserUIExptManager::ResetBrowserUIStateForAllProfiles() {
  CHECK(observation_.IsObserving());
  observation_.Reset();

  // Reset all currently active normal profiles.
  for (auto* profile : profile_manager_->GetLoadedProfiles()) {
    if (!profile->IsRegularProfile()) {
      continue;
    }

    ResetForDayZeroBrowserUI(profile);
  }
}

void DayZeroBrowserUIExptManager::OnP3AEnabledChanged(
    const std::string& pref_names) {
  IsP3AEnabled() ? SetDayZeroBrowserUIForAllProfiles()
                 : ResetBrowserUIStateForAllProfiles();
}

bool DayZeroBrowserUIExptManager::IsP3AEnabled() const {
  return p3a_enabled_.GetValue();
}

void DayZeroBrowserUIExptManager::SetDayZeroBrowserUIForAllProfiles() {
  for (auto* profile : profile_manager_->GetLoadedProfiles()) {
    if (!profile->IsRegularProfile()) {
      continue;
    }

    SetForDayZeroBrowserUI(profile);
  }
}
