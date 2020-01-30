// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_sponsored_images/browser/view_counter_service.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_sponsored_images/browser/features.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace ntp_sponsored_images {

namespace {

NTPSponsoredImagesData* GetDemoWallpaper() {
  static auto demo = std::make_unique<NTPSponsoredImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  demo->wallpaper_image_files = {
      base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")),
      base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")),
      base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")),
  };
  demo->logo_alt_text = "Technikke: For music lovers.";
  demo->logo_company_name = "Technikke";
  demo->logo_destination_url = "https://brave.com";
  return demo.get();
}

}  // namespace

// static
void ViewCounterService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kBrandedWallpaperNotificationDismissed, false);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowBrandedBackgroundImage, true);
}

ViewCounterService::ViewCounterService(
    NTPSponsoredImagesService* service,
    PrefService* prefs,
    bool is_supported_locale)
    : service_(service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale) {
  // If we have a wallpaper, store it as private var.
  // Set demo wallpaper if a flag is set.
  if (!base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    DCHECK(service_);
    service_->AddObserver(this);
  }

  if (current_wallpaper()) {
    model_.set_total_image_count(
        current_wallpaper()->wallpaper_image_files.size());
  }

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(brave_ads::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
}

ViewCounterService::~ViewCounterService() = default;

NTPSponsoredImagesData* ViewCounterService::current_wallpaper() {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    return GetDemoWallpaper();
  }
  return service_->GetSponsoredImagesData();
}

void ViewCounterService::Shutdown() {
  if (service_ && service_->HasObserver(this))
    service_->RemoveObserver(this);
}

void ViewCounterService::OnUpdated(
    NTPSponsoredImagesData* data) {
  DCHECK(
      !base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo));
  DCHECK(service_);

  // Data is updated, so change our stored data and reset any indexes.
  // But keep view counter until branded content is seen.
  model_.ResetCurrentWallpaperImageIndex();
  model_.set_total_image_count(data ? data->wallpaper_image_files.size() : 0);
}


void ViewCounterService::OnPreferenceChanged(
    const std::string& pref_name) {
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

bool ViewCounterService::IsBrandedWallpaperActive() {
  return (is_supported_locale_ && IsOptedIn() && current_wallpaper() &&
      current_wallpaper()->IsValid());
}

bool ViewCounterService::ShouldShowBrandedWallpaper() {
  return IsBrandedWallpaperActive() && model_.ShouldShowBrandedWallpaper();
}

size_t ViewCounterService::GetWallpaperImageIndexToDisplay() {
  return model_.current_wallpaper_image_index();
}

bool ViewCounterService::IsOptedIn() {
  return prefs_->GetBoolean(prefs::kNewTabPageShowBrandedBackgroundImage) &&
         prefs_->GetBoolean(kNewTabPageShowBackgroundImage);
}

}  // namespace ntp_sponsored_images
