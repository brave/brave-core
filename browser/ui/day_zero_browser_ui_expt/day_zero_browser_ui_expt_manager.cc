/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/day_zero_browser_ui_expt/day_zero_browser_ui_expt_manager.h"

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/content/common/pref_names.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace {
constexpr int kDayZeroFeatureDurationInDays = 1;
}  // namespace

// static
std::unique_ptr<DayZeroBrowserUIExptManager>
DayZeroBrowserUIExptManager::Create(ProfileManager* profile_manager) {
  if (!base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)) {
    return nullptr;
  }

  // This class should be instantiated after getting valid first run time;
  if (brave_stats::GetFirstRunTime(nullptr).is_null()) {
    // This should not be happened in production but not 100% in the wild(ex,
    // corrupted user data). Just early return for safe. If upstream changes the
    // timing of fetching first run time, browser test will catch this.
    LOG(ERROR) << __func__ << " This should be happened only on test.";
    return nullptr;
  }

  // If one day passed since first run, we don't need to touch original default
  // pref values. Just early return and this class is no-op.
  if (base::Time::Now() - brave_stats::GetFirstRunTime(nullptr) >=
      base::Days(kDayZeroFeatureDurationInDays)) {
    VLOG(2) << __func__ << " Already passed day zero feature duration.";
    return nullptr;
  }

  // base::WrapUnique for using private ctor.
  return base::WrapUnique(new DayZeroBrowserUIExptManager(profile_manager));
}

DayZeroBrowserUIExptManager::DayZeroBrowserUIExptManager(
    ProfileManager* profile_manager,
    std::optional<base::Time> mock_first_run_time)
    : profile_manager_(*profile_manager),
      first_run_time_for_testing_(mock_first_run_time) {
  for (auto* profile : profile_manager_->GetLoadedProfiles()) {
    if (!profile->IsRegularProfile()) {
      continue;
    }

    SetForDayZeroBrowserUI(profile);
  }

  observation_.Observe(&(*profile_manager_));
  StartResetTimer();
}

DayZeroBrowserUIExptManager::~DayZeroBrowserUIExptManager() {
  if (observation_.IsObserving()) {
    observation_.Reset();
  }
}

void DayZeroBrowserUIExptManager::OnProfileAdded(Profile* profile) {
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
  prefs->SetDefaultPrefValue(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             base::Value(false));
  prefs->SetDefaultPrefValue(brave_rewards::prefs::kShowLocationBarButton,
                             base::Value(false));
  prefs->SetDefaultPrefValue(brave_news::prefs::kNewTabPageShowToday,
                             base::Value(false));
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

void DayZeroBrowserUIExptManager::StartResetTimer() {
  auto remained_time_to_expt_duration_since_first_run =
      GetFirstRunTime() - base::Time::Now();

  // Convenient switch only for testing purpose.
  constexpr char kUseShortExpirationForDayZeroExpt[] =
      "use-short-expiration-for-day-zero-expt";
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kUseShortExpirationForDayZeroExpt)) {
    remained_time_to_expt_duration_since_first_run += base::Minutes(2);
  } else {
    remained_time_to_expt_duration_since_first_run +=
        base::Days(kDayZeroFeatureDurationInDays);
  }

  // If remained time is negative, reset to original here.
  if (remained_time_to_expt_duration_since_first_run < base::TimeDelta()) {
    ResetBrowserUIStateForAllProfiles();
    return;
  }

  reset_timer_.Start(
      FROM_HERE, remained_time_to_expt_duration_since_first_run, this,
      &DayZeroBrowserUIExptManager::ResetBrowserUIStateForAllProfiles);
}

base::Time DayZeroBrowserUIExptManager::GetFirstRunTime() const {
  if (first_run_time_for_testing_) {
    return *first_run_time_for_testing_;
  }

  return brave_stats::GetFirstRunTime(nullptr);
}
