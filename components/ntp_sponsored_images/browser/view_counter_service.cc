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
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_service.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/sponsored_view_counter_model.h"
#include "brave/components/ntp_sponsored_images/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace ntp_sponsored_images {

// static
void ViewCounterService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      prefs::kBrandedWallpaperNotificationDismissed, false);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowBrandedBackgroundImage, true);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowReferralBackgroundImage, true);
}

ViewCounterService::ViewCounterService(
    NTPReferralImagesService* referral_service,
    NTPSponsoredImagesService* sponsored_service,
    PrefService* prefs,
    bool is_supported_locale)
    : referral_service_(referral_service),
      sponsored_service_(sponsored_service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale) {
  // If we have a wallpaper, store it as private var.
  // Set demo wallpaper if a flag is set.
  if (sponsored_service_ &&
      !base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    sponsored_service_->AddObserver(this);
  }

  if (referral_service_)
    referral_service_->AddObserver(this);

  model_.reset(new SponsoredViewCounterModel);
  if (auto* data = GetCurrentSponsoredWallpaperData())
    model_->set_total_image_count(data->wallpaper_image_files.size());

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(brave_ads::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
}

ViewCounterService::~ViewCounterService() = default;

NTPSponsoredImagesData*
ViewCounterService::GetCurrentSponsoredWallpaperData() const {
  if (!sponsored_service_)
    return nullptr;

  return sponsored_service_->GetSponsoredImagesData();
}

base::Value ViewCounterService::GetCurrentWallpaper() const {
  if (ShouldShowReferralWallpaper()) {
    return GetCurrentReferralWallpaperData()->GetValueAt(
        model_->current_wallpaper_image_index());
  }

  if (ShouldShowSponsoredWallpaper()) {
    return GetCurrentSponsoredWallpaperData()->GetValueAt(
        model_->current_wallpaper_image_index());
  }

  return base::Value();
}

void ViewCounterService::Shutdown() {
  if (sponsored_service_ && sponsored_service_->HasObserver(this))
    sponsored_service_->RemoveObserver(this);
}

void ViewCounterService::OnReferralImagesUpdated(NTPReferralImagesData* data) {
  DCHECK(referral_service_);

  if (data && IsReferralWallpaperActive())
    ResetViewCounterModelByDataUpdated(data->wallpaper_image_files.size());
}

void ViewCounterService::OnSponsoredImagesUpdated(
    NTPSponsoredImagesData* data) {
  DCHECK(
      !base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo));
  DCHECK(sponsored_service_);

  if (data && IsSponsoredWallpaperActive())
    ResetViewCounterModelByDataUpdated(data->wallpaper_image_files.size());
}

void ViewCounterService::ResetViewCounterModelByDataUpdated(
    int background_images_count) {
  model_->ResetCurrentWallpaperImageIndex();
  model_->set_total_image_count(background_images_count);
}

void ViewCounterService::OnPreferenceChanged() {
  ResetNotificationState();
}

void ViewCounterService::ResetNotificationState() {
  prefs_->SetBoolean(prefs::kBrandedWallpaperNotificationDismissed, false);
}

void ViewCounterService::RegisterPageView() {
  // Don't do any counting if we will never be showing the data
  // since we want the count to start at the point of data being available
  // or the user opt-in status changing.
  if (IsReferralWallpaperActive() || IsSponsoredWallpaperActive())
    model_->RegisterPageView();
}

bool ViewCounterService::ShouldShowSponsoredWallpaper() const {
  return IsSponsoredWallpaperActive() && model_->ShouldShowWallpaper();
}

bool ViewCounterService::IsSponsoredWallpaperActive() const {
  return is_supported_locale_ && show_background_image_enabled_ &&
         IsSponsoredWallpaperOptedIn() && GetCurrentSponsoredWallpaperData();
}

bool ViewCounterService::IsSponsoredWallpaperOptedIn() const {
  return prefs_->GetBoolean(prefs::kNewTabPageShowBrandedBackgroundImage);
}

bool ViewCounterService::ShouldShowReferralWallpaper() const {
  return IsReferralWallpaperActive() && model_->ShouldShowWallpaper();
}

bool ViewCounterService::IsReferralWallpaperActive() const {
  return show_background_image_enabled_ &&
      IsReferralWallpaperOptedIn() && GetCurrentReferralWallpaperData();
}

bool ViewCounterService::IsReferralWallpaperOptedIn() const {
  return prefs_->GetBoolean(prefs::kNewTabPageShowReferralBackgroundImage);
}

NTPReferralImagesData*
ViewCounterService::GetCurrentReferralWallpaperData() const {
  if (!referral_service_)
    return nullptr;
  return referral_service_->GetReferralImagesData();
}

}  // namespace ntp_sponsored_images
