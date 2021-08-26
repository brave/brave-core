// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace content {
class WebUIDataSource;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class WeeklyStorage;

namespace ntp_background_images {

#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
struct NTPBackgroundImagesData;
#endif
struct NTPSponsoredImagesData;
struct TopSite;

class ViewCounterService : public KeyedService,
                           public NTPBackgroundImagesService::Observer {
 public:
  ViewCounterService(NTPBackgroundImagesService* service,
                     brave_ads::AdsService* ads_service,
                     PrefService* prefs,
                     PrefService* local_state,
                     bool is_supported_locale);
  ~ViewCounterService() override;

  ViewCounterService(const ViewCounterService&) = delete;
  ViewCounterService& operator=(const ViewCounterService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Lets the counter know that a New Tab Page view has occured.
  // This should always be called as it will evaluate whether the user has
  // opted-in or data is available.
  void RegisterPageView();

  void BrandedWallpaperLogoClicked(const std::string& creative_instance_id,
                                   const std::string& destination_url,
                                   const std::string& wallpaper_id);

  base::Value GetCurrentWallpaperForDisplay() const;
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
  base::Value GetCurrentWallpaper() const;
#endif
  base::Value GetCurrentBrandedWallpaper() const;
  std::vector<TopSite> GetTopSitesVectorForWebUI() const;
  std::vector<TopSite> GetTopSitesVectorData() const;

  bool IsSuperReferral() const;
  std::string GetSuperReferralThemeName() const;
  std::string GetSuperReferralCode() const;

  void BrandedWallpaperWillBeDisplayed(const std::string& wallpaper_id);

#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
  NTPBackgroundImagesData* GetCurrentWallpaperData() const;
#endif
  // Gets the current data for branded wallpaper, if there
  // is a wallpaper active. Does not consider user opt-in
  // status, or consider whether the wallpaper should be shown.
  NTPSponsoredImagesData* GetCurrentBrandedWallpaperData() const;

  void InitializeWebUIDataSource(content::WebUIDataSource* html_source);

 private:
  // Sync with themeValues in brave_appearance_page.js
  enum ThemesOption {
    DEFAULT,
    SUPER_REFERRAL,
  };

  friend class NTPBackgroundImagesViewCounterTest;
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           SINotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           SINotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveInitiallyOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveOptedInWithNTPBackgoundOption);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithNTPBackgoundOptionOptedOut);
#endif

  void OnPreferenceChanged(const std::string& pref_name);

  // KeyedService
  void Shutdown() override;

  // NTPBackgroundImagesService::Observer
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
  void OnUpdated(NTPBackgroundImagesData* data) override;
#endif
  void OnUpdated(NTPSponsoredImagesData* data) override;
  void OnSuperReferralEnded() override;

  void ResetNotificationState();
  bool IsSponsoredImagesWallpaperOptedIn() const;
  bool IsSuperReferralWallpaperOptedIn() const;
  // Do we have a sponsored or referral wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool IsBrandedWallpaperActive() const;
#if BUILDFLAG(ENABLE_NTP_BACKGROUND_IMAGES)
  // Should we show background image.
  bool IsBackgroundWallpaperActive() const;
#endif
  // Should we show the branded wallpaper right now, in addition
  // to the result from `IsBrandedWallpaperActive()`.
  bool ShouldShowBrandedWallpaper() const;

  void ResetModel();

  void UpdateP3AValues() const;

  NTPBackgroundImagesService* service_ = nullptr;  // not owned
  brave_ads::AdsService* ads_service_ = nullptr;  // not owned
  PrefService* prefs_ = nullptr;  // not owned
  bool is_supported_locale_ = false;
  PrefChangeRegistrar pref_change_registrar_;
  ViewCounterModel model_;

  // If P3A is enabled, these will track number of tabs created
  // and the ratio of those which are branded images.
  std::unique_ptr<WeeklyStorage> new_tab_count_state_;
  std::unique_ptr<WeeklyStorage> branded_new_tab_count_state_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_

