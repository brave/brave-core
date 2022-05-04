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
#include "base/metrics/histogram_macros.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"
#endif

namespace {

constexpr char kNewTabsCreated[] = "brave.new_tab_page.p3a_new_tabs_created";
constexpr char kSponsoredNewTabsCreated[] =
    "brave.new_tab_page.p3a_sponsored_new_tabs_created";

}  // namespace

namespace ntp_background_images {

// static
void ViewCounterService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kNewTabsCreated);
  registry->RegisterListPref(kSponsoredNewTabsCreated);
}

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

ViewCounterService::ViewCounterService(
    NTPBackgroundImagesService* service,
    NTPCustomBackgroundImagesService* custom_service,
    brave_ads::AdsService* ads_service,
    PrefService* prefs,
    PrefService* local_state,
    bool is_supported_locale)
    : service_(service),
      ads_service_(ads_service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale),
      custom_bi_service_(custom_service) {
  DCHECK(service_);
  service_->AddObserver(this);

  new_tab_count_state_ =
      std::make_unique<WeeklyStorage>(local_state, kNewTabsCreated);
  branded_new_tab_count_state_ =
      std::make_unique<WeeklyStorage>(local_state, kSponsoredNewTabsCreated);

  ResetModel();

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(ads::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(prefs::kNewTabPageSuperReferralThemesOption,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
      base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowBackgroundImage,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          base::Unretained(this)));

  OnUpdated(GetCurrentBrandedWallpaperData());
  OnUpdated(GetCurrentWallpaperData());
}

ViewCounterService::~ViewCounterService() = default;

void ViewCounterService::BrandedWallpaperWillBeDisplayed(
    const std::string& wallpaper_id) {
  if (ads_service_) {
    base::Value data = ViewCounterService::GetCurrentWallpaperForDisplay();
    DCHECK(!data.is_none());

    const std::string* creative_instance_id =
        data.FindStringKey(kCreativeInstanceIDKey);
    ads_service_->TriggerNewTabPageAdEvent(
        wallpaper_id, creative_instance_id ? *creative_instance_id : "",
        ads::mojom::NewTabPageAdEventType::kViewed);
  }

  branded_new_tab_count_state_->AddDelta(1);
  UpdateP3AValues();
}

NTPBackgroundImagesData* ViewCounterService::GetCurrentWallpaperData() const {
  return service_->GetBackgroundImagesData();
}

NTPSponsoredImagesData* ViewCounterService::GetCurrentBrandedWallpaperData()
    const {
  auto* sr_data = service_->GetBrandedImagesData(true /* for_sr */);
  if (sr_data && IsSuperReferralWallpaperOptedIn())
    return sr_data;

  return service_->GetBrandedImagesData(false);
}

base::Value ViewCounterService::GetCurrentWallpaperForDisplay() const {
  if (ShouldShowBrandedWallpaper()) {
    return GetCurrentBrandedWallpaper();
  } else {
    return GetCurrentWallpaper();
  }
}

base::Value ViewCounterService::GetCurrentWallpaper() const {
  if (!IsBackgroundWallpaperActive())
    return base::Value();

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  if (custom_bi_service_ && custom_bi_service_->ShouldShowCustomBackground())
    return custom_bi_service_->GetBackground();
#endif

  return GetCurrentWallpaperData()->GetBackgroundAt(
      model_.current_wallpaper_image_index());
}

base::Value ViewCounterService::GetCurrentBrandedWallpaper() const {
  if (GetCurrentBrandedWallpaperData()) {
    size_t current_campaign_index;
    size_t current_background_index;
    std::tie(current_campaign_index, current_background_index) =
        model_.GetCurrentBrandedImageIndex();
    return GetCurrentBrandedWallpaperData()->GetBackgroundAt(
        current_campaign_index, current_background_index);
  }

  return base::Value();
}

std::vector<TopSite> ViewCounterService::GetTopSitesData() const {
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
  DVLOG(2) << __func__ << ": Active data is updated.";

  // Data is updated, reset any indexes.
  if (data) {
    ResetModel();
  }
}

void ViewCounterService::OnUpdated(NTPSponsoredImagesData* data) {
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

  if (data) {
    ResetModel();
  }
}

void ViewCounterService::OnSuperReferralEnded() {
  // Need to reset model because SI images are shown only for every 4th NTP but
  // we've shown SR images for every NTP.
  ResetModel();
}

void ViewCounterService::ResetModel() {
  model_.Reset();

  model_.set_show_branded_wallpaper(IsSponsoredImagesWallpaperOptedIn());
  model_.set_show_wallpaper(
      prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage));

  // SR/SI
  if (auto* data = GetCurrentBrandedWallpaperData()) {
    std::vector<size_t> campaigns_total_branded_images_count;
    for (const auto& campaign : data->campaigns) {
      campaigns_total_branded_images_count.push_back(
          campaign.backgrounds.size());
    }
    model_.SetCampaignsTotalBrandedImageCount(
        campaigns_total_branded_images_count);
    model_.set_always_show_branded_wallpaper(data->IsSuperReferral());
  }
  // BI
  if (auto* data = GetCurrentWallpaperData()) {
    model_.set_total_image_count(data->backgrounds.size());
  }
}

void ViewCounterService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == ads::prefs::kEnabled) {
    ResetNotificationState();
    return;
  }

  // Reset model because SI and SR use different policy.
  // Start from initial model state whenever
  // prefs::kNewTabPageSuperReferralThemesOption or
  // prefs::kNewTabPageShowSponsoredImagesBackgroundImage prefs are changed.
  ResetModel();
}

void ViewCounterService::ResetNotificationState() {
  prefs_->SetBoolean(prefs::kBrandedWallpaperNotificationDismissed, false);
}

void ViewCounterService::RegisterPageView() {
  new_tab_count_state_->AddDelta(1);
  UpdateP3AValues();
  // This will be no-op when component is not ready.
  service_->CheckNTPSIComponentUpdateIfNeeded();
  model_.RegisterPageView();
}

void ViewCounterService::BrandedWallpaperLogoClicked(
    const std::string& creative_instance_id,
    const std::string& destination_url,
    const std::string& wallpaper_id) {
  if (!ads_service_)
    return;

  ads_service_->TriggerNewTabPageAdEvent(
      wallpaper_id, creative_instance_id,
      ads::mojom::NewTabPageAdEventType::kClicked);
}

bool ViewCounterService::ShouldShowBrandedWallpaper() const {
  return IsBrandedWallpaperActive() && model_.ShouldShowBrandedWallpaper();
}

void ViewCounterService::InitializeWebUIDataSource(
    content::WebUIDataSource* html_source) {
  html_source->AddString("superReferralThemeName", GetSuperReferralThemeName());
}

bool ViewCounterService::IsBrandedWallpaperActive() const {
  if (!GetCurrentBrandedWallpaperData()) {
    return false;
  }

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

bool ViewCounterService::IsBackgroundWallpaperActive() const {
#if !BUILDFLAG(IS_ANDROID)
  if (!prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage))
    return false;
#endif

  return !!GetCurrentWallpaperData();
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

void ViewCounterService::UpdateP3AValues() const {
  uint64_t new_tab_count = new_tab_count_state_->GetHighestValueInWeek();
  constexpr int kNewTabCount[] = {0, 3, 8, 20, 50, 100};
  const int* it_count =
      std::lower_bound(kNewTabCount, std::end(kNewTabCount), new_tab_count);
  int answer = it_count - kNewTabCount;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.NTP.NewTabsCreated", answer,
                             std::size(kNewTabCount) + 1);

  constexpr double kSponsoredRatio[] = {0, 10.0, 20.0, 30.0, 40.0, 50.0};
  uint64_t branded_new_tab_count =
      branded_new_tab_count_state_->GetHighestValueInWeek();
  if (branded_new_tab_count == 0 || new_tab_count == 0) {
    answer = 0;
  } else {
    double ratio = (branded_new_tab_count /
                    static_cast<double>(new_tab_count)) * 100;
    const double* it_ratio =
        std::lower_bound(kSponsoredRatio, std::end(kSponsoredRatio), ratio);
    answer = it_ratio - kSponsoredRatio;
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.NTP.SponsoredNewTabsCreated", answer,
                             std::size(kSponsoredRatio) + 1);
}

}  // namespace ntp_background_images
