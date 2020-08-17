// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"

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

  if (auto* data = GetCurrentBrandedWallpaperData())
    model_.set_total_image_count(data->backgrounds.size());

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(brave_rewards::prefs::kBraveRewardsEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(brave_ads::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(prefs::kNewTabPageSuperReferralThemesOption,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));

  OnUpdated(GetCurrentBrandedWallpaperData());
}

ViewCounterService::~ViewCounterService() = default;

NTPBackgroundImagesData*
ViewCounterService::GetCurrentBrandedWallpaperData() const {
  auto* sr_data = service_->GetBackgroundImagesData(true /* for_sr */);
  if (sr_data && IsSuperReferralWallpaperOptedIn())
    return sr_data;

  return service_->GetBackgroundImagesData(false);
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

base::Value ViewCounterService::GetTopSites(bool for_webui) const {
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (auto* data = GetCurrentBrandedWallpaperData()) {
    if (data->IsSuperReferral())
      return GetCurrentBrandedWallpaperData()->GetTopSites(for_webui);
  }
#endif

  return base::Value();
}

std::vector<TopSite> ViewCounterService::GetTopSitesVectorData() const {
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  if (auto* data = GetCurrentBrandedWallpaperData())
    return data->top_sites;
#endif

  return {};
}

void ViewCounterService::Shutdown() {
  service_->RemoveObserver(this);
}

void ViewCounterService::OnUpdated(NTPBackgroundImagesData* data) {
  // We can get non effective component update because
  // NTPBackgroundImagesService just notifies whenever any component is updated.
  // When SR component is ended, |data| is for SR but
  // GetCurrentBrandedWallpaperData() will return data for SI.
  // When it happens, this callback can't update model_ properly because it
  // returns early by below if clause. But, we have to reset |model_| because
  // SR and SI uses different model_ policy. OnSuperReferralEnded() will handle
  // it instead.
  if (data != GetCurrentBrandedWallpaperData())
    return;

  DVLOG(2) << __func__ << ": Active data is updated.";

  // Data is updated, so change our stored data and reset any indexes.
  // But keep view counter until branded content is seen.
  if (data) {
    model_.ResetCurrentWallpaperImageIndex();
    model_.set_total_image_count(data->backgrounds.size());
    model_.set_ignore_count_to_branded_wallpaper(data->IsSuperReferral());
  }
}

void ViewCounterService::OnSuperReferralEnded() {
  // Need to reset model because SI images are shown only for every 4th NTP but
  // we've shown SR images for every NTP.
  ResetModel();
}

void ViewCounterService::ResetModel() {
  if (auto* data = GetCurrentBrandedWallpaperData()) {
    model_.Reset(false /* use_initial_count */);
    model_.set_total_image_count(data->backgrounds.size());
    model_.set_ignore_count_to_branded_wallpaper(data->IsSuperReferral());
  }
}

void ViewCounterService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == prefs::kNewTabPageSuperReferralThemesOption) {
    // Reset model because SI and SR use different policy.
    ResetModel();
    return;
  }

  // Other prefs changes are used for notification state.
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

void ViewCounterService::InitializeWebUIDataSource(
    content::WebUIDataSource* html_source) {
  html_source->AddString("superReferralThemeName", GetSuperReferralThemeName());
}

bool ViewCounterService::IsBrandedWallpaperActive() const {
  if (!GetCurrentBrandedWallpaperData())
    return false;

  // We show SR regardless of ntp background images option because SR works
  // like theme.
  if (GetCurrentBrandedWallpaperData()->IsSuperReferral() &&
      IsSuperReferralWallpaperOptedIn())
    return true;

  // We don't show SI if user disables bg image.
  if (!prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage))
    return false;

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

bool ViewCounterService::IsSuperReferral() const {
  return service_->IsSuperReferral();
}

std::string ViewCounterService::GetSuperReferralThemeName() const {
  return service_->GetSuperReferralThemeName();
}

std::string ViewCounterService::GetSuperReferralCode() const {
  return service_->GetSuperReferralCode();
}

}  // namespace ntp_background_images
