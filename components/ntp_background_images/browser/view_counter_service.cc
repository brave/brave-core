// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "url/gurl.h"

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
    std::unique_ptr<NTPP3AHelper> ntp_p3a_helper,
    bool is_supported_locale)
    : service_(service),
      ads_service_(ads_service),
      prefs_(prefs),
      is_supported_locale_(is_supported_locale),
      custom_bi_service_(custom_service),
      ntp_p3a_helper_(std::move(ntp_p3a_helper)) {
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
    const std::string* wallpaper_id,
    const std::string* creative_instance_id) {
  if (ads_service_) {
    ads_service_->TriggerNewTabPageAdEvent(
        wallpaper_id ? *wallpaper_id : "",
        creative_instance_id ? *creative_instance_id : "",
        ads::mojom::NewTabPageAdEventType::kViewed);

    if (ntp_p3a_helper_ && !ads_service_->IsEnabled()) {
      // Should only report to P3A if ads are disabled, as required by spec.
      ntp_p3a_helper_->RecordView(creative_instance_id ? *creative_instance_id
                                                       : "");
    }
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

absl::optional<base::Value::Dict>
ViewCounterService::GetCurrentWallpaperForDisplay() {
  if (ShouldShowBrandedWallpaper()) {
    absl::optional<base::Value::Dict> branded_wallpaper =
        GetCurrentBrandedWallpaper();
    if (branded_wallpaper) {
      return branded_wallpaper;
    }
    // Failed to get branded wallpaper as it was frequency capped by ads
    // service. In this case next background wallpaper should be shown on NTP.
    // To do this we need to increment background wallpaper index as it wasn't
    // incremented during the last RegisterPageView() call.
    model_.IncreaseBackgroundWallpaperImageIndex();
  }

  return GetCurrentWallpaper();
}

absl::optional<base::Value::Dict> ViewCounterService::GetCurrentWallpaper()
    const {
  if (!IsBackgroundWallpaperActive())
    return absl::nullopt;

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  if (ShouldShowCustomBackground()) {
    if (auto background = custom_bi_service_->GetBackground();
        !background.empty()) {
      return background;
    }
  }
#endif

  auto* data = GetCurrentWallpaperData();
  if (!data) {
    CHECK_IS_TEST();
    return absl::nullopt;
  }

  auto background =
      data->GetBackgroundAt(model_.current_wallpaper_image_index());
  background.Set(kWallpaperRandomKey, true);
  return background;
}

absl::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaper() const {
  NTPSponsoredImagesData* images_data = GetCurrentBrandedWallpaperData();
  if (!images_data) {
    return absl::nullopt;
  }

  const bool should_frequency_cap_ads =
      ads_service_ && ads_service_->IsEnabled();
  if (should_frequency_cap_ads && !images_data->IsSuperReferral()) {
    return GetCurrentBrandedWallpaperByAdInfo();
  }

  return GetCurrentBrandedWallpaperFromModel();
}

absl::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaperByAdInfo() const {
  DCHECK(ads_service_);

  absl::optional<ads::NewTabPageAdInfo> ad_info =
      ads_service_->GetPrefetchedNewTabPageAd();
  if (!ad_info) {
    return absl::nullopt;
  }

  absl::optional<base::Value::Dict> branded_wallpaper_data =
      GetCurrentBrandedWallpaperData()->GetBackgroundByAdInfo(*ad_info);
  if (!branded_wallpaper_data) {
    ads_service_->OnFailedToPrefetchNewTabPageAd(ad_info->placement_id,
                                                 ad_info->creative_instance_id);
  }

  return branded_wallpaper_data;
}

absl::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaperFromModel() const {
  size_t current_campaign_index;
  size_t current_background_index;
  std::tie(current_campaign_index, current_background_index) =
      model_.GetCurrentBrandedImageIndex();
  return GetCurrentBrandedWallpaperData()->GetBackgroundAt(
      current_campaign_index, current_background_index);
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
    model_.set_always_show_branded_wallpaper(data->IsSuperReferral());
    model_.SetCampaignsTotalBrandedImageCount(
        campaigns_total_branded_images_count);
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
  MaybePrefetchNewTabPageAd();
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

  if (ntp_p3a_helper_ && !ads_service_->IsEnabled()) {
    // Should only report to P3A if ads are disabled, as required by spec.
    ntp_p3a_helper_->RecordClickAndMaybeLand(creative_instance_id);
  }
}

bool ViewCounterService::ShouldShowBrandedWallpaper() const {
  return IsBrandedWallpaperActive() && model_.ShouldShowBrandedWallpaper();
}

bool ViewCounterService::ShouldShowCustomBackground() const {
#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  return custom_bi_service_ && custom_bi_service_->ShouldShowCustomBackground();
#else
  return false;
#endif
}

void ViewCounterService::InitializeWebUIDataSource(
    content::WebUIDataSource* html_source) {
  html_source->AddString("superReferralThemeName", GetSuperReferralThemeName());
}

void ViewCounterService::OnTabURLChanged(const GURL& url) {
  if (ntp_p3a_helper_) {
    ntp_p3a_helper_->SetLastTabURL(url);
  }
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

  return !!GetCurrentWallpaperData() || ShouldShowCustomBackground();
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

void ViewCounterService::MaybePrefetchNewTabPageAd() {
  NTPSponsoredImagesData* images_data = GetCurrentBrandedWallpaperData();
  if (!IsBrandedWallpaperActive() || !ads_service_ ||
      !ads_service_->IsEnabled() || !images_data ||
      images_data->IsSuperReferral()) {
    return;
  }

  ads_service_->PrefetchNewTabPageAd();
}

void ViewCounterService::UpdateP3AValues() const {
  uint64_t new_tab_count = new_tab_count_state_->GetHighestValueInWeek();
  p3a_utils::RecordToHistogramBucket("Brave.NTP.NewTabsCreated",
                                     {0, 3, 8, 20, 50, 100}, new_tab_count);

  constexpr char kSponsoredNewTabsHistogramName[] =
      "Brave.NTP.SponsoredNewTabsCreated";
  constexpr int kSponsoredRatio[] = {0, 10, 20, 30, 40, 50};
  uint64_t branded_new_tab_count =
      branded_new_tab_count_state_->GetHighestValueInWeek();
  if (branded_new_tab_count == 0 || new_tab_count == 0) {
    UMA_HISTOGRAM_EXACT_LINEAR(kSponsoredNewTabsHistogramName, 0,
                               std::size(kSponsoredRatio) + 1);
  } else {
    double ratio = (branded_new_tab_count /
                    static_cast<double>(new_tab_count)) * 100;
    p3a_utils::RecordToHistogramBucket(kSponsoredNewTabsHistogramName,
                                       kSponsoredRatio,
                                       static_cast<int>(ratio));
  }
}

}  // namespace ntp_background_images
