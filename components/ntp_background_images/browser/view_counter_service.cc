// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/debug/crash_logging.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_util.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_theme_option_type.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr char kNewTabsCreatedHistogramName[] = "Brave.NTP.NewTabsCreated.3";
constexpr int kNewTabsCreatedMetricBuckets[] = {0, 1, 2, 3, 4, 8, 15};
constexpr char kSponsoredNewTabsHistogramName[] =
    "Brave.NTP.SponsoredNewTabsCreated.2";
constexpr int kSponsoredNewTabsBuckets[] = {0, 10, 20, 30, 40, 50};

constexpr base::TimeDelta kP3AReportInterval = base::Days(1);

}  // namespace

ViewCounterService::ViewCounterService(
    HostContentSettingsMap* host_content_settings_map,
    NTPBackgroundImagesService* background_images_service,
    BraveNTPCustomBackgroundService* custom_background_service,
    brave_ads::AdsService* ads_service,
    PrefService* prefs,
    PrefService* local_state,
    std::unique_ptr<NTPP3AHelper> ntp_p3a_helper,
    bool is_supported_locale)
    : host_content_settings_map_(host_content_settings_map),
      background_images_service_(background_images_service),
      ads_service_(ads_service),
      prefs_(prefs),
      local_state_(local_state),
      is_supported_locale_(is_supported_locale),
      model_(prefs),
      custom_background_service_(custom_background_service),
      ntp_p3a_helper_(std::move(ntp_p3a_helper)) {
  DCHECK(background_images_service_);
  ntp_background_images_service_observation_.Observe(
      background_images_service_);

  if (ads_service_) {
    ads_service_->AddObserver(this);
  }

  host_content_settings_map_->AddObserver(this);

  new_tab_count_state_ =
      std::make_unique<WeeklyStorage>(local_state, prefs::kNewTabsCreated);
  branded_new_tab_count_state_ = std::make_unique<WeeklyStorage>(
      local_state, prefs::kSponsoredNewTabsCreated);

  ResetModel();

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kNewTabPageSuperReferralThemesOption,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kNewTabPageShowBackgroundImage,
      base::BindRepeating(&ViewCounterService::OnPreferenceChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  OnBackgroundImagesDataDidUpdate(
      background_images_service_->GetBackgroundImagesData());
  OnSponsoredImagesDataDidUpdate(GetSponsoredImagesData());

  UpdateP3AValues();
}

ViewCounterService::~ViewCounterService() {
  if (ads_service_) {
    ads_service_->RemoveObserver(this);
  }

  host_content_settings_map_->RemoveObserver(this);
}

void ViewCounterService::OnDidInitializeAdsService() {
  background_images_service_->RegisterSponsoredImagesComponent();
}

void ViewCounterService::OnDidClearAdsServiceData() {
  background_images_service_->ForceSponsoredComponentUpdate();
}

void ViewCounterService::OnContentSettingChanged(
    const ContentSettingsPattern& /*primary_pattern*/,
    const ContentSettingsPattern& /*secondary_pattern*/,
    ContentSettingsTypeSet content_type_set) {
  if (content_type_set.Contains(ContentSettingsType::JAVASCRIPT)) {
    ResetModel();
  }
}

void ViewCounterService::RecordViewedAdEvent(
    const std::string& placement_id,
    const std::string& campaign_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type) {
  if (ntp_p3a_helper_ &&
      mojom_ad_metric_type == brave_ads::mojom::NewTabPageAdMetricType::kP3A) {
    ntp_p3a_helper_->RecordView(creative_instance_id, campaign_id);
  }
  branded_new_tab_count_state_->AddDelta(1);
  UpdateP3AValues();

  // Ads component skips confirmations for P3A and disabled metrics. Still
  // trigger the ad event so dependent logic runs.
  MaybeTriggerNewTabPageAdEvent(
      placement_id, creative_instance_id, mojom_ad_metric_type,
      brave_ads::mojom::NewTabPageAdEventType::kViewedImpression);
}

void ViewCounterService::RecordClickedAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const std::string& /*target_url*/,
    brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type) {
  if (ntp_p3a_helper_ &&
      mojom_ad_metric_type == brave_ads::mojom::NewTabPageAdMetricType::kP3A) {
    ntp_p3a_helper_->RecordNewTabPageAdEvent(
        brave_ads::mojom::NewTabPageAdEventType::kClicked,
        creative_instance_id);
  }

  // Ads component skips confirmations for P3A and disabled metrics. Still
  // trigger the ad event so dependent logic runs.
  MaybeTriggerNewTabPageAdEvent(
      placement_id, creative_instance_id, mojom_ad_metric_type,
      brave_ads::mojom::NewTabPageAdEventType::kClicked);
}

NTPSponsoredImagesData* ViewCounterService::GetSponsoredImagesData() const {
  const bool supports_rich_media =
      host_content_settings_map_->GetDefaultContentSetting(
          ContentSettingsType::JAVASCRIPT) == CONTENT_SETTING_ALLOW;

  NTPSponsoredImagesData* images_data =
      background_images_service_->GetSponsoredImagesData(
          /*super_referral=*/true, supports_rich_media);
  if (images_data && IsSuperReferralWallpaperOptedIn()) {
    return images_data;
  }

  return background_images_service_->GetSponsoredImagesData(
      /*super_referral=*/false, supports_rich_media);
}

std::optional<base::Value::Dict>
ViewCounterService::GetNextWallpaperForDisplay() {
  model_.RotateBackgroundWallpaperImageIndex();
  return GetCurrentWallpaper();
}

std::optional<base::Value::Dict>
ViewCounterService::GetCurrentWallpaperForDisplay() {
  if (ShouldShowSponsoredImages()) {
    if (std::optional<base::Value::Dict> dict = GetCurrentBrandedWallpaper()) {
      return dict;
    }
  }

  // If a sponsored image should not be displayed, fallback to the next
  // wallpaper.
  return GetNextWallpaperForDisplay();
}

std::optional<base::Value::Dict> ViewCounterService::GetCurrentWallpaper()
    const {
  if (!CanShowBackgroundImages()) {
    return std::nullopt;
  }

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  if (ShouldShowCustomBackgroundImages()) {
    if (auto background = custom_background_service_->GetBackground();
        !background.empty()) {
      return background;
    }
  }
#endif

  const NTPBackgroundImagesData* const images_data =
      background_images_service_->GetBackgroundImagesData();
  if (!images_data) {
    CHECK_IS_TEST();
    return std::nullopt;
  }

  return images_data->GetBackgroundAt(model_.current_wallpaper_image_index())
      .Set(kWallpaperRandomKey, true);
}

std::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaper() const {
  NTPSponsoredImagesData* images_data = GetSponsoredImagesData();
  if (!images_data) {
    return std::nullopt;
  }

  if (images_data->IsSuperReferral()) {
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Super referrals are enabled");
    DUMP_WILL_BE_NOTREACHED();
    return GetCurrentBrandedWallpaperFromModel();
  }

  return GetCurrentBrandedWallpaperFromAdsService();
}

std::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaperFromAdsService() const {
  DCHECK(ads_service_);

  const std::optional<brave_ads::NewTabPageAdInfo> ad =
      ads_service_->MaybeGetPrefetchedNewTabPageAd();
  if (!ad) {
    return std::nullopt;
  }

  NTPSponsoredImagesData* images_data = GetSponsoredImagesData();
  if (!images_data) {
    ads_service_->OnFailedToPrefetchNewTabPageAd(ad->placement_id,
                                                 ad->creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              ad->creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "No sponsored images data after prefetching");
    DUMP_WILL_BE_NOTREACHED();
    return std::nullopt;
  }

  std::optional<base::Value::Dict> background =
      images_data->MaybeGetBackground(*ad);
  if (!background) {
    ads_service_->OnFailedToPrefetchNewTabPageAd(ad->placement_id,
                                                 ad->creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              ad->creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "No matching background");
    DUMP_WILL_BE_NOTREACHED();
    return std::nullopt;
  }

  return background;
}

std::optional<base::Value::Dict>
ViewCounterService::GetCurrentBrandedWallpaperFromModel() const {
  const auto [campaign_index, creative_index] =
      model_.GetCurrentBrandedImageIndex();
  return GetSponsoredImagesData()->MaybeGetBackgroundAt(campaign_index,
                                                        creative_index);
}

std::vector<TopSite> ViewCounterService::GetTopSitesData() const {
  if (const NTPSponsoredImagesData* const images_data =
          GetSponsoredImagesData()) {
    return images_data->top_sites;
  }

  return {};
}

void ViewCounterService::Shutdown() {
  ntp_background_images_service_observation_.Reset();
}

void ViewCounterService::OnBackgroundImagesDataDidUpdate(
    NTPBackgroundImagesData* data) {
  if (data) {
    DVLOG(2) << __func__ << ": NTP BI component is updated.";
    ResetModel();
  }
}

void ViewCounterService::OnSponsoredImagesDataDidUpdate(
    NTPSponsoredImagesData* data) {
  if (data) {
    DVLOG(2) << __func__ << ": NTP SI/SR component is updated.";
    ResetModel();
  }
}

void ViewCounterService::OnSponsoredContentDidUpdate(
    const base::Value::Dict& data) {
  if (ads_service_) {
    // Since `data` contains small JSON from a CRX component, cloning it has no
    // performance impact.
    ads_service_->ParseAndSaveNewTabPageAds(
        data.Clone(),
        base::BindOnce(&ViewCounterService::ParseAndSaveNewTabPageAdsCallback,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void ViewCounterService::OnSuperReferralCampaignDidEnd() {
  // Need to reset model because SI images are shown only for every 4th NTP but
  // we've shown SR images for every NTP.
  ResetModel();
}

void ViewCounterService::ParseAndSaveNewTabPageAdsCallback(bool success) {
  if (success) {
    MaybePrefetchNewTabPageAd();
  } else {
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Failed to parse and save ads");
    DUMP_WILL_BE_NOTREACHED();
  }
}

void ViewCounterService::ResetModel() {
  model_.Reset();

  model_.set_show_branded_wallpaper(IsSponsoredImagesWallpaperOptedIn());
  model_.set_show_wallpaper(IsShowBackgroundImageOptedIn());

  if (const NTPSponsoredImagesData* const images_data =
          GetSponsoredImagesData()) {
    std::vector<size_t> campaigns_total_branded_images_count;
    campaigns_total_branded_images_count.reserve(images_data->campaigns.size());
    for (const auto& campaign : images_data->campaigns) {
      campaigns_total_branded_images_count.push_back(campaign.creatives.size());
    }
    model_.set_always_show_branded_wallpaper(images_data->IsSuperReferral());
    model_.SetCampaignsTotalBrandedImageCount(
        campaigns_total_branded_images_count);
  }

  if (const NTPBackgroundImagesData* const images_data =
          background_images_service_->GetBackgroundImagesData()) {
    model_.set_total_image_count(
        static_cast<int>(images_data->backgrounds.size()));
  }
}

void ViewCounterService::OnPreferenceChanged(const std::string& pref_name) {
  if (pref_name == brave_rewards::prefs::kEnabled) {
    ResetNotificationState();
    return;
  }

  if (pref_name == prefs::kNewTabPageShowBackgroundImage ||
      pref_name == prefs::kNewTabPageShowSponsoredImagesBackgroundImage) {
    RecordSponsoredImagesEnabledP3A(prefs_);
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
  background_images_service_->MaybeCheckForSponsoredComponentUpdate();
  model_.RegisterPageView();
  MaybePrefetchNewTabPageAd();
}

bool ViewCounterService::ShouldShowSponsoredImages() const {
  return CanShowSponsoredImages() && model_.ShouldShowSponsoredImages();
}

bool ViewCounterService::ShouldShowCustomBackgroundImages() const {
#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  return custom_background_service_ &&
         custom_background_service_->ShouldShowCustomBackground();
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
    ntp_p3a_helper_->OnNavigationDidFinish(url);
  }
}

NTPP3AHelper* ViewCounterService::GetP3AHelper() const {
  return ntp_p3a_helper_.get();
}

bool ViewCounterService::CanShowSponsoredImages() const {
  NTPSponsoredImagesData* images_data = GetSponsoredImagesData();
  if (!images_data) {
    return false;
  }

  if (images_data->IsSuperReferral() && IsSuperReferralWallpaperOptedIn()) {
    // Super referral is always shown if opted in.
    return true;
  }

  if (!IsShowBackgroundImageOptedIn()) {
    return false;
  }

  return IsSponsoredImagesWallpaperOptedIn();
}

bool ViewCounterService::CanShowBackgroundImages() const {
#if !BUILDFLAG(IS_ANDROID)
  if (!IsShowBackgroundImageOptedIn()) {
    return false;
  }
#endif

  return !!background_images_service_->GetBackgroundImagesData() ||
         ShouldShowCustomBackgroundImages();
}

bool ViewCounterService::IsShowBackgroundImageOptedIn() const {
  return prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage);
}

bool ViewCounterService::IsSponsoredImagesWallpaperOptedIn() const {
  return prefs_->GetBoolean(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage) &&
        is_supported_locale_;
}

bool ViewCounterService::IsSuperReferralWallpaperOptedIn() const {
  return prefs_->GetInteger(prefs::kNewTabPageSuperReferralThemesOption) ==
         static_cast<int>(ThemesOption::kSuperReferral);
}

bool ViewCounterService::IsSuperReferral() const {
  return background_images_service_->IsSuperReferral();
}

std::string ViewCounterService::GetSuperReferralThemeName() const {
  return background_images_service_->GetSuperReferralThemeName();
}

std::string ViewCounterService::GetSuperReferralCode() const {
  return background_images_service_->GetSuperReferralCode();
}

void ViewCounterService::MaybePrefetchNewTabPageAd() {
  NTPSponsoredImagesData* images_data = GetSponsoredImagesData();
  if (!ads_service_) {
    return;
  }

  if (!CanShowSponsoredImages() || !images_data) {
    return;
  }

  if (images_data->IsSuperReferral()) {
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Super referrals are enabled");
    DUMP_WILL_BE_NOTREACHED();
    return;
  }

  ads_service_->PrefetchNewTabPageAd();
}

void ViewCounterService::MaybeTriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type) {
  if (ads_service_) {
    ads_service_->TriggerNewTabPageAdEvent(
        placement_id, creative_instance_id, mojom_ad_metric_type,
        mojom_ad_event_type,
        base::BindOnce(
            [](const std::string& creative_instance_id,
               brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
               brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
               bool success) {
              if (!success) {
                SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                                          creative_instance_id);
                SCOPED_CRASH_KEY_NUMBER("Issue50267", "metric_type",
                                        static_cast<int>(mojom_ad_metric_type));
                SCOPED_CRASH_KEY_NUMBER("Issue50267", "event_type",
                                        static_cast<int>(mojom_ad_event_type));
                SCOPED_CRASH_KEY_STRING64(
                    "Issue50267", "failure_reason",
                    "Failed to trigger new tab page ad event");
                DUMP_WILL_BE_NOTREACHED();
              }
            },
            creative_instance_id, mojom_ad_metric_type, mojom_ad_event_type));
  }
}

void ViewCounterService::UpdateP3AValues() {
  uint64_t new_tab_count = new_tab_count_state_->GetHighestValueInWeek();
  p3a_utils::RecordToHistogramBucket(kNewTabsCreatedHistogramName,
                                     kNewTabsCreatedMetricBuckets,
                                     static_cast<int>(new_tab_count));

  uint64_t branded_new_tab_count =
      branded_new_tab_count_state_->GetHighestValueInWeek();
  if (branded_new_tab_count == 0 || new_tab_count == 0) {
    UMA_HISTOGRAM_EXACT_LINEAR(kSponsoredNewTabsHistogramName, 0,
                               std::size(kSponsoredNewTabsBuckets) + 1);
  } else {
    double ratio = (static_cast<double>(branded_new_tab_count) /
                    static_cast<double>(new_tab_count)) *
                   100;
    p3a_utils::RecordToHistogramBucket(kSponsoredNewTabsHistogramName,
                                       kSponsoredNewTabsBuckets,
                                       static_cast<int>(ratio));
  }

  p3a_update_timer_.Start(FROM_HERE, base::Time::Now() + kP3AReportInterval,
                          base::BindOnce(&ViewCounterService::UpdateP3AValues,
                                         weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace ntp_background_images
