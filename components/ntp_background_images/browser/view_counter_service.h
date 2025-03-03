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

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
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

class HostContentSettingsMap;
class WeeklyStorage;

namespace ntp_background_images {

class BraveNTPCustomBackgroundService;
class NTPP3AHelper;

struct NTPBackgroundImagesData;
struct NTPSponsoredImagesData;
struct TopSite;

class ViewCounterService : public KeyedService,
                           public content_settings::Observer,
                           public NTPBackgroundImagesService::Observer {
 public:
  ViewCounterService(HostContentSettingsMap* host_content_settings,
                     NTPBackgroundImagesService* background_images_service,
                     BraveNTPCustomBackgroundService* custom_background_service,
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

  void MaybeTriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type);

  std::optional<base::Value::Dict> GetNextWallpaperForDisplay();
  std::optional<base::Value::Dict> GetCurrentWallpaperForDisplay();
  std::optional<base::Value::Dict> GetCurrentWallpaper() const;
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaper();
  void GetCurrentBrandedWallpaper(
      base::OnceCallback<
          void(const std::optional<GURL>& url,
               const std::optional<std::string>& placement_id,
               const std::optional<std::string>& creative_instance_id,
               const std::optional<GURL>& target_url)> callback) const;
  std::optional<brave_ads::ConditionMatcherMap> GetConditionMatchers(
      const base::Value::Dict& dict);
  std::optional<base::Value::Dict>
  GetNextBrandedWallpaperWhichMatchesConditions();
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaperFromAdInfo() const;
  std::optional<base::Value::Dict> GetCurrentBrandedWallpaperFromModel() const;
  std::vector<TopSite> GetTopSitesData() const;

  bool IsSuperReferral() const;
  std::string GetSuperReferralThemeName() const;
  std::string GetSuperReferralCode() const;

  void BrandedWallpaperWillBeDisplayed(const std::string& wallpaper_id,
                                       const std::string& creative_instance_id,
                                       const std::string& campaign_id);
  NTPSponsoredImagesData* GetSponsoredImagesData() const;

  void InitializeWebUIDataSource(content::WebUIDataSource* html_source);

  void OnTabURLChanged(const GURL& url);

 private:
  // Sync with themeValues in brave_appearance_page.js
  enum ThemesOption {
    DEFAULT,
    SUPER_REFERRAL,
  };

  friend class ViewCounterServiceTest;
  friend class NTPBackgroundImagesServiceTest;
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, CanShowSponsoredImages);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, CannotShowSponsoredImages);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToAllowed);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      BlockNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToBlocked);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeOverWithImageIfJavaScriptContentSettingIsSetToAllowed);
  FRIEND_TEST_ALL_PREFIXES(
      ViewCounterServiceTest,
      AllowNewTabTakeoverWithImageIfJavaScriptContentSettingIsSetToBlocked);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowSponsoredImagesIfUninitialized);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowSponsoredImagesIfMalformed);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowSponsoredImagesIfOptedOut);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, ActiveInitiallyOptedIn);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           ActiveOptedInWithNTPBackgoundOption);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, ModelTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, CanShowBackgroundImages);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, CannotShowBackgroundImages);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowBackgroundImagesIfUninitialized);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowBackgroundImagesIfMalformed);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest,
                           CannotShowBackgroundImagesIfOptedOut);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, PrefsWithModelTest);
  FRIEND_TEST_ALL_PREFIXES(ViewCounterServiceTest, GetCurrentWallpaper);

  void OnPreferenceChanged(const std::string& pref_name);

  // content_settings::Observer:
  void OnContentSettingChanged(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsTypeSet content_type_set) override;

  // KeyedService:
  void Shutdown() override;

  // NTPBackgroundImagesService::Observer:
  void OnBackgroundImagesDataDidUpdate(NTPBackgroundImagesData* data) override;
  void OnSponsoredImagesDataDidUpdate(NTPSponsoredImagesData* data) override;
  void OnSponsoredContentDidUpdate(const base::Value::Dict& data) override;
  void OnSuperReferralCampaignDidEnd() override;

  void ParseAndSaveCreativeNewTabPageAdsCallback(bool success);

  void ResetNotificationState();
  bool IsSponsoredImagesWallpaperOptedIn() const;
  bool IsSuperReferralWallpaperOptedIn() const;

  // Do we have a sponsored or referral wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool CanShowSponsoredImages() const;
  // Should we show the branded wallpaper right now, in addition to the result
  // from `CanShowSponsoredImages()`.
  bool ShouldShowSponsoredImages() const;

  bool CanShowBackgroundImages() const;

  // Should we show custom background that user uploaded or fill background
  // with solid color user selected.
  bool ShouldShowCustomBackground() const;

  void ResetModel();

  void MaybePrefetchNewTabPageAd();

  void UpdateP3AValues();

  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  raw_ptr<NTPBackgroundImagesService> background_images_service_ = nullptr;
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<PrefService> local_state_prefs_ = nullptr;
  bool is_supported_locale_ = false;
  PrefChangeRegistrar pref_change_registrar_;
  ViewCounterModel model_;
  base::WallClockTimer p3a_update_timer_;
  std::optional<base::Value::Dict> current_wallpaper_;

  // Can be null if custom background is not supported.
  raw_ptr<BraveNTPCustomBackgroundService> custom_background_service_ = nullptr;

  // If P3A is enabled, these will track number of tabs created
  // and the ratio of those which are branded images.
  std::unique_ptr<WeeklyStorage> new_tab_count_state_;
  std::unique_ptr<WeeklyStorage> branded_new_tab_count_state_;

  std::unique_ptr<NTPP3AHelper> ntp_p3a_helper_;
  base::ScopedObservation<NTPBackgroundImagesService,
                          NTPBackgroundImagesService::Observer>
      ntp_background_images_service_observation_{this};

  base::WeakPtrFactory<ViewCounterService> weak_ptr_factory_{this};
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
