/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_NEW_TAB_TAKEOVER_WALLPAPER_PROVIDER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_NEW_TAB_TAKEOVER_WALLPAPER_PROVIDER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider.h"

class HostContentSettingsMap;
class PrefService;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace ntp_background_images {

class NTPBackgroundImagesService;
class ViewCounterModel;
struct NTPSponsoredContentData;

// Whether the resulting takeover is a static or dynamic new tab takeover
// creative is decided by the ad server response, not by this provider.
class NTPNewTabTakeoverWallpaperProvider final : public NTPWallpaperProvider {
 public:
  NTPNewTabTakeoverWallpaperProvider(
      NTPBackgroundImagesService& background_images_service,
      ViewCounterModel& view_counter_model,
      HostContentSettingsMap& host_content_settings_map,
      PrefService& prefs,
      brave_ads::AdsService& ads_service,
      bool is_supported_locale);

  NTPNewTabTakeoverWallpaperProvider(
      const NTPNewTabTakeoverWallpaperProvider&) = delete;
  NTPNewTabTakeoverWallpaperProvider& operator=(
      const NTPNewTabTakeoverWallpaperProvider&) = delete;

  ~NTPNewTabTakeoverWallpaperProvider() override;

  // NTPWallpaperProvider:
  bool IsEligible() const override;
  void MaybeGetWallpaper(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback)
      override;

 private:
  NTPSponsoredContentData* GetNewTabTakeover() const;
  bool IsShowBackgroundImageOptedIn() const;
  bool CanShowNewTabTakeoverWallpaper() const;

  void MaybeServeNewTabPageAdCallback(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
      brave_ads::mojom::NewTabPageAdInfoPtr ad);
  void CheckCreativeFileExists(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
      base::DictValue creative_dict);
  void CheckCreativeFileExistsCallback(
      NTPWallpaperProvider::MaybeGetWallpaperCallback callback,
      base::DictValue creative_dict,
      bool file_exists);

  const raw_ref<NTPBackgroundImagesService> background_images_service_;
  const raw_ref<ViewCounterModel> view_counter_model_;
  const raw_ref<HostContentSettingsMap> host_content_settings_map_;
  const raw_ref<PrefService> prefs_;
  const raw_ref<brave_ads::AdsService> ads_service_;
  const bool is_supported_locale_;

  base::WeakPtrFactory<NTPNewTabTakeoverWallpaperProvider> weak_ptr_factory_{
      this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_NEW_TAB_TAKEOVER_WALLPAPER_PROVIDER_H_
