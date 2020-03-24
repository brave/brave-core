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
      prefs::kNewTabPageShowBrandedBackgroundImage, true);
}

ViewCounterService::ViewCounterService(NTPBackgroundImagesService* service,
                                       PrefService* prefs,
                                       bool is_supported_locale)
    : service_(service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale) {
  DCHECK(service_);
  // If we have a wallpaper, store it as private var.
  // Set demo wallpaper if a flag is set.
  if (!base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    service_->AddObserver(this);
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
    return GetCurrentBrandedWallpaperData()->GetValueAt(
        model_.current_wallpaper_image_index());
  }

  return base::Value();
}

void ViewCounterService::Shutdown() {
  if (!base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo))
    service_->RemoveObserver(this);
}

void ViewCounterService::OnUpdated(NTPBackgroundImagesData* data) {
  DCHECK(
      !base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo));
  // Data is updated, so change our stored data and reset any indexes.
  // But keep view counter until branded content is seen.
  model_.ResetCurrentWallpaperImageIndex();
  model_.set_total_image_count(data ? data->backgrounds.size() : 0);
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
  return is_supported_locale_ && IsBrandedWallpaperOptedIn() &&
         GetCurrentBrandedWallpaperData();
}

bool ViewCounterService::IsBrandedWallpaperOptedIn() const {
  return prefs_->GetBoolean(prefs::kNewTabPageShowBrandedBackgroundImage) &&
         prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage);
}

}  // namespace ntp_background_images
