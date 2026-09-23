/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_new_tab_takeover_wallpaper_provider.h"

#include <utility>

#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"

#include "brave/components/brave_ads/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

namespace ntp_background_images {

// New Tab Takeover shows a sponsored creative instead of a regular background.
// This class owns deciding whether one is eligible and fetching it.

NTPNewTabTakeoverWallpaperProvider::NTPNewTabTakeoverWallpaperProvider(
    NTPBackgroundImagesService& background_images_service,
    ViewCounterModel& view_counter_model,
    HostContentSettingsMap& host_content_settings_map,
    PrefService& prefs,
    brave_ads::AdsService& ads_service,
    bool is_supported_locale)
    : background_images_service_(background_images_service),
      view_counter_model_(view_counter_model),
      host_content_settings_map_(host_content_settings_map),
      prefs_(prefs),
      ads_service_(ads_service),
      is_supported_locale_(is_supported_locale) {}

NTPNewTabTakeoverWallpaperProvider::~NTPNewTabTakeoverWallpaperProvider() =
    default;

bool NTPNewTabTakeoverWallpaperProvider::IsEligible() const {
  return GetSponsoredImagesData() && IsShowBackgroundImageOptedIn() &&
         IsSponsoredImagesWallpaperOptedIn() &&
         view_counter_model_->ShouldShowSponsoredImages();
}

void NTPNewTabTakeoverWallpaperProvider::MaybeGetWallpaper(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback) {
  if (!IsEligible()) {
    return std::move(callback).Run(std::nullopt);
  }

  ads_service_->MaybeServeNewTabPageAd(base::BindOnce(
      &NTPNewTabTakeoverWallpaperProvider::MaybeServeNewTabPageAdCallback,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

NTPSponsoredImagesData* NTPNewTabTakeoverWallpaperProvider::
    GetSponsoredImagesData() const {
  const bool supports_rich_media =
      host_content_settings_map_->GetDefaultContentSetting(
          ContentSettingsType::JAVASCRIPT) == CONTENT_SETTING_ALLOW;
  return background_images_service_->GetSponsoredImagesData(
      supports_rich_media);
}

bool NTPNewTabTakeoverWallpaperProvider::IsShowBackgroundImageOptedIn()
    const {
  return prefs_->GetBoolean(prefs::kNewTabPageShowBackgroundImage);
}

bool NTPNewTabTakeoverWallpaperProvider::IsSponsoredImagesWallpaperOptedIn()
    const {
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  return prefs_->GetBoolean(brave_ads::prefs::kSponsoredEnabled) &&
         is_supported_locale_;
#else
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
}

void NTPNewTabTakeoverWallpaperProvider::MaybeServeNewTabPageAdCallback(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
    brave_ads::mojom::NewTabPageAdInfoPtr ad) {
  if (!ad) {
    return std::move(callback).Run(std::nullopt);
  }

  NTPSponsoredImagesData* const sponsored_images_data =
      GetSponsoredImagesData();
  if (!sponsored_images_data) {
    return std::move(callback).Run(std::nullopt);
  }

  std::optional<base::DictValue> creative_dict =
      sponsored_images_data->MaybeGetBackground(*ad);
  if (!creative_dict) {
    return std::move(callback).Run(std::nullopt);
  }

  CheckCreativeFileExists(std::move(callback), std::move(*creative_dict));
}

void NTPNewTabTakeoverWallpaperProvider::CheckCreativeFileExists(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
    base::DictValue creative_dict) {
  const std::string* const file_path =
      creative_dict.FindString(kWallpaperFilePathKey);
  if (!file_path) {
    SCOPED_CRASH_KEY_STRING64(
        "Issue55874", "failure_reason",
        "Missing file path for New Tab Takeover creative");
    DUMP_WILL_BE_NOTREACHED();
    return std::move(callback).Run(std::nullopt);
  }

  const base::FilePath creative_file_path =
      base::FilePath::FromUTF8Unsafe(*file_path);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&base::PathExists, creative_file_path),
      base::BindOnce(&NTPNewTabTakeoverWallpaperProvider::
                          CheckCreativeFileExistsCallback,
                      weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                      std::move(creative_dict)));
}

void NTPNewTabTakeoverWallpaperProvider::CheckCreativeFileExistsCallback(
    NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
    base::DictValue creative_dict,
    bool file_exists) {
  if (!file_exists) {
    if (const std::string* const creative_instance_id =
            creative_dict.FindString(kCreativeInstanceIDKey)) {
      SCOPED_CRASH_KEY_STRING64("Issue55874", "creative_instance_id",
                                *creative_instance_id);
    }
    SCOPED_CRASH_KEY_STRING64(
        "Issue55874", "failure_reason",
        "Creative file is missing when the New Tab Takeover is shown");
    DUMP_WILL_BE_NOTREACHED();
    return std::move(callback).Run(std::nullopt);
  }

  std::move(callback).Run(std::move(creative_dict));
}

}  // namespace ntp_background_images
