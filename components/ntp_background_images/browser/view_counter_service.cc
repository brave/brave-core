// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

// static
void ViewCounterService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kBrandedWallpaperNotificationDismissed, false);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage, true);
  // Integer type is used because this pref is used by radio button group in
  // appearance settings. Super referral is disabled when it is set to DEFAULT.
  registry->RegisterIntegerPref(
      prefs::kNewTabPageSuperReferralThemesOption, SUPER_REFERRAL);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowBackgroundImage, true);
}

ViewCounterService::ViewCounterService(NTPBackgroundImagesService* service,
                                       PrefService* prefs,
                                       bool is_supported_locale)
    : service_(service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale) {
  DCHECK(service_);
  service_->AddObserver(this);
  if (service_->test_data_used()) {
    // Explicitly trigger OnUpdated() because test data can be set before
    // Observeris added to |service_|.
    OnUpdated(service_->GetBackgroundImagesData());
  }

  if (auto* data = GetCurrentBrandedWallpaperData())
    model_.set_total_image_count(data->backgrounds.size());

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(brave_ads::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
}

ViewCounterService::~ViewCounterService() = default;

NTPBackgroundImagesData*
ViewCounterService::GetCurrentBrandedWallpaperData() const {
  return service_->GetBackgroundImagesData();
}

base::Value ViewCounterService::GetCurrentWallpaperForDisplay() const {
  if (ShouldShowBrandedWallpaper()) {
    return GetCurrentWallpaper();
  }

  return base::Value();
}

base::Value ViewCounterService::GetCurrentWallpaper() const {
  if (GetCurrentBrandedWallpaperData()) {
    return GetCurrentBrandedWallpaperData()->GetBackgroundAt(
        model_.current_wallpaper_image_index());
  }

  return base::Value();
}

base::Value ViewCounterService::GetTopSites() const {
  if (ShouldShowBrandedWallpaper())
    return GetCurrentBrandedWallpaperData()->GetTopSites();

  return base::Value();
}

void ViewCounterService::Shutdown() {
  service_->RemoveObserver(this);
}

void ViewCounterService::OnUpdated(NTPBackgroundImagesData* data) {
  // Data is updated, so change our stored data and reset any indexes.
  // But keep view counter until branded content is seen.
  if (data) {
    model_.ResetCurrentWallpaperImageIndex();
    model_.set_total_image_count(data->backgrounds.size());
    model_.set_ignore_count_to_branded_wallpaper(data->IsSuperReferral());
  }
}

void ViewCounterService::OnPreferenceChanged(const std::string& pref_name) {
  ResetNotificationState();
}

void ViewCounterService::ResetNotificationState() {
  prefs_->SetBoolean(prefs::kBrandedWallpaperNotificationDismissed, false);
}

void ViewCounterService::RegisterPageView() {
  // Don't do any counting if we will never be showing the data
  // since we want the count to start at the point of data being available
  // or the user opt-in status changing.
  if (IsBrandedWallpaperActive()) {
    model_.RegisterPageView();
  }
}

bool ViewCounterService::ShouldShowBrandedWallpaper() const {
  return IsBrandedWallpaperActive() && model_.ShouldShowBrandedWallpaper();
}

bool ViewCounterService::IsBrandedWallpaperActive() const {
  if (!prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage))
    return false;

  if (!GetCurrentBrandedWallpaperData())
    return false;

  if (GetCurrentBrandedWallpaperData()->IsSuperReferral())
    return IsSuperReferralWallpaperOptedIn();

  return IsSponsoredImagesWallpaperOptedIn();
}

bool ViewCounterService::IsSponsoredImagesWallpaperOptedIn() const {
  return prefs_->GetBoolean(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage) &&
        is_supported_locale_;
}

bool ViewCounterService::IsSuperReferralWallpaperOptedIn() const {
  return prefs_->GetInteger(prefs::kNewTabPageSuperReferralThemesOption) ==
             SUPER_REFERRAL;
}

}  // namespace ntp_background_images
