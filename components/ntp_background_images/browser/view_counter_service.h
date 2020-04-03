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
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace ntp_background_images {

struct NTPBackgroundImagesData;
struct TopSite;

class ViewCounterService : public KeyedService,
                           public NTPBackgroundImagesService::Observer {
 public:
  ViewCounterService(NTPBackgroundImagesService* service,
                     PrefService* prefs,
                     bool is_supported_locale);
  ~ViewCounterService() override;

  ViewCounterService(const ViewCounterService&) = delete;
  ViewCounterService& operator=(const ViewCounterService&) = delete;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Lets the counter know that a New Tab Page view has occured.
  // This should always be called as it will evaluate whether the user has
  // opted-in or data is available.
  void RegisterPageView();

  base::Value GetCurrentWallpaperForDisplay() const;
  base::Value GetCurrentWallpaper() const;
  base::Value GetTopSites(bool for_webui = false) const;
  std::vector<TopSite> GetTopSitesVectorData() const;

  // This api can be used for fast checking before SR component registration.
  // NOTE: SR Data could not be availble even if this returns true.
  // Use this api just for checking whether this install is SR.
  // This returns true if we certainly know this install is SR.
  // If this returns false, we don't know this install is SR or not for now.
  bool IsSuperReferral() const;

  // Gets the current data for branded wallpaper, if there
  // is a wallpaper active. Does not consider user opt-in
  // status, or consider whether the wallpaper should be shown.
  NTPBackgroundImagesData* GetCurrentBrandedWallpaperData() const;

 private:
  // Sync with themeValues in brave_appearance_page.js
  enum ThemesOption {
    DEFAULT,
    SUPER_REFERRAL,
  };

  friend class NTPBackgroundImagesViewCounterTest;
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           NotActiveOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPBackgroundImagesViewCounterTest,
                           ActiveInitiallyOptedIn);

  void OnPreferenceChanged(const std::string& pref_name);

  // KeyedService
  void Shutdown() override;

  // NTPBackgroundImagesService::Observer
  void OnUpdated(NTPBackgroundImagesData* data) override;

  void ResetNotificationState();
  bool IsSponsoredImagesWallpaperOptedIn() const;
  bool IsSuperReferralWallpaperOptedIn() const;
  // Do we have a sponsored or referral wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool IsBrandedWallpaperActive() const;
  // Should we show the branded wallpaper right now, in addition
  // to the result from `IsBrandedWallpaperActive()`.
  bool ShouldShowBrandedWallpaper() const;

  void ResetModel();

  NTPBackgroundImagesService* service_ = nullptr;  // not owned
  PrefService* prefs_ = nullptr;  // not owned
  bool is_supported_locale_ = false;
  PrefChangeRegistrar pref_change_registrar_;
  ViewCounterModel model_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_

