// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

class GURL;

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace content {
class WebUIDataSource;
}  // namespace content

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class WeeklyStorage;

namespace ntp_background_images {

class BraveNTPCustomBackgroundService;
class NTPP3AHelper;

struct NTPBackgroundImagesData;
struct NTPSponsoredImagesData;
struct TopSite;

class ViewCounterService : public KeyedService,
                           public NTPBackgroundImagesService::Observer {
 public:
  ViewCounterService(NTPBackgroundImagesService* service,
                     BraveNTPCustomBackgroundService* custom_service,
                     brave_ads::AdsService* ads_service,
                     PrefService* prefs,
                     PrefService* local_state,
                     std::unique_ptr<NTPP3AHelper> ntp_p3a_helper,
                     bool is_supported_locale);
  ~ViewCounterService() override;

  ViewCounterService(const ViewCounterService&) = delete;
  ViewCounterService& operator=(const ViewCounterService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static void RegisterProfilePrefsForMigration(
      user_prefs::PrefRegistrySyncable* registry);
  static void MigrateObsoleteProfilePrefs(PrefService* prefs);

  // Lets the counter know that a New Tab Page view has occured.
  // This should always be called as it will evaluate whether the user has
  // opted-in or data is available.
  void RegisterPageView();

  void BrandedWallpaperLogoClicked(const std::string& creative_instance_id,
                                   const std::string& destination_url,
                                   const std::string& wallpaper_id);

  std::optional<base::Value::Dict> GetNextWallpaperForDisplay();
  std::optional<base::Value::Dict> GetCurrentWallpaperForDisplay();
  std::optional<base::Value::Dict> GetCurrentWallpaper() const;
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaper();
  std::optional<brave_ads::NewTabPageAdConditionMatcherMap>
  GetConditionMatchers(const base::Value::Dict& dict);
  std::optional<base::Value::Dict>
  GetNextBrandedWallpaperWhichMatchesConditions();
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaperFromAdInfo() const;
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaperFromModel() const;
  std::vector<TopSite> GetTopSitesData() const;

  bool IsSuperReferral() const;
  std::string GetSuperReferralThemeName() const;
  std::string GetSuperReferralCode() const;

  void BrandedWallpaperWillBeDisplayed(const std::string& wallpaper_id,
                                       const std::string& creative_instance_id);

  NTPBackgroundImagesData* GetCurrentWallpaperData() const;
  // Gets the current data for branded wallpaper, if there
  // is a wallpaper active. Does not consider user opt-in
  // status, or consider whether the wallpaper should be shown.
  NTPSponsoredImagesData* GetCurrentBrandedWallpaperData() const;

  void InitializeWebUIDataSource(content::WebUIDataSource* html_source);

  void OnTabURLChanged(const GURL& url);

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
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveInitiallyOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveOptedInWithNTPBackgoundOption);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           BINotActiveWithNTPBackgoundOptionOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           PrefsWithModelTest);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           GetCurrentWallpaperTest);

  void OnPreferenceChanged(const std::string& pref_name);

  // KeyedService
  void Shutdown() override;

  // NTPBackgroundImagesService::Observer
  void OnUpdated(NTPBackgroundImagesData* data) override;
  void OnUpdated(NTPSponsoredImagesData* data) override;
  void OnSuperReferralEnded() override;

  void ResetNotificationState();
  bool IsSponsoredImagesWallpaperOptedIn() const;
  bool IsSuperReferralWallpaperOptedIn() const;
  // Do we have a sponsored or referral wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool IsBrandedWallpaperActive() const;
  // Should we show background image.
  bool IsBackgroundWallpaperActive() const;
  // Should we show the branded wallpaper right now, in addition
  // to the result from `IsBrandedWallpaperActive()`.
  bool ShouldShowBrandedWallpaper() const;
  // Should we show custom background that user uploaded or fill background
  // with solid color user selected.
  bool ShouldShowCustomBackground() const;

  void ResetModel();

  void MaybePrefetchNewTabPageAd();

  void UpdateP3AValues();

  raw_ptr<NTPBackgroundImagesService> service_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<PrefService> local_state_prefs_ = nullptr;
  bool is_supported_locale_ = false;
  PrefChangeRegistrar pref_change_registrar_;
  ViewCounterModel model_;
  base::WallClockTimer p3a_update_timer_;

  // Can be null if custom background is not supported.
  raw_ptr<BraveNTPCustomBackgroundService> custom_bi_service_ = nullptr;

  // If P3A is enabled, these will track number of tabs created
  // and the ratio of those which are branded images.
  std::unique_ptr<WeeklyStorage> new_tab_count_state_;
  std::unique_ptr<WeeklyStorage> branded_new_tab_count_state_;

  std::unique_ptr<NTPP3AHelper> ntp_p3a_helper_;
  base::ScopedObservation<NTPBackgroundImagesService,
                          NTPBackgroundImagesService::Observer>
      ntp_background_images_service_observation_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
