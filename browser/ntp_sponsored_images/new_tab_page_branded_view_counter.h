// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NEW_TAB_PAGE_BRANDED_VIEW_COUNTER_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NEW_TAB_PAGE_BRANDED_VIEW_COUNTER_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_component_manager.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_data.h"

class Profile;

class NewTabPageBrandedViewCounter
    : public KeyedService,
      public NTPSponsoredImagesComponentManager::Observer,
      public brave_rewards::RewardsServiceObserver {
 public:
  static void EnsureBrowserContextKeyedServiceFactoriesBuilt();
  static NewTabPageBrandedViewCounter* GetForProfile(Profile* profile);

  explicit NewTabPageBrandedViewCounter(Profile* profile);
  ~NewTabPageBrandedViewCounter() override;

  // Lets the counter know that a New Tab Page view has occured.
  // This should always be called as it will evaluate whether the user has
  // opted-in or data is available.
  void RegisterPageView();
  // Do we have a branded wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool IsBrandedWallpaperActive();
  // Should we show the branded wallpaper right now, in addition
  // to the result from `IsBrandedWallpaperActive()`.
  bool ShouldShowBrandedWallpaper();
  // Gets the current data for branded wallpaper, if there
  // is a wallpaper active. Does not consider user opt-in
  // status, or consider whether the wallpaper should be shown.
  const NTPSponsoredImagesData& GetBrandedWallpaper();
  size_t GetWallpaperImageIndexToDisplay();

 private:
  // brave_rewards::RewardsServiceObserver
  void OnRewardsMainEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool rewards_main_enabled) override;
  void OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) override;

  // KeyedService
  void Shutdown() override;

  // NTPSponsoredImagesComponentManager::Observer
  void OnUpdated(const NTPSponsoredImagesData& data) override;

  bool GetBrandedWallpaperFromDataSource();
  void SetShouldShowFromPreferences();
  void ResetNotificationState();

  std::unique_ptr<NTPSponsoredImagesData> current_wallpaper_ = nullptr;
  size_t current_wallpaper_image_index_ = 0;
  bool has_user_opted_in_;
  bool is_supported_locale_;
  int count_to_branded_wallpaper_;
  PrefChangeRegistrar pref_change_registrar_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NewTabPageBrandedViewCounter);
};

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NEW_TAB_PAGE_BRANDED_VIEW_COUNTER_H_
