// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_service.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace ntp_sponsored_images {

struct NTPReferralImagesData;
struct NTPSponsoredImagesData;

class ViewCounterModel;

// ViewCounterService chooses and provide wallpaper data among two different
// data sources(Referral or Sponsored).
class ViewCounterService : public KeyedService,
                           public NTPReferralImagesService::Observer,
                           public NTPSponsoredImagesService::Observer {
 public:
  ViewCounterService(NTPReferralImagesService* referral_service,
                     NTPSponsoredImagesService* sponsored_service,
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

  base::Value GetCurrentWallpaper() const;
  void set_show_background_image_enabled(bool enabled) {
    show_background_image_enabled_ = enabled;
  }

 private:
  enum class ActiveDataType {
    TYPE_SPONSORED,
    TYPE_REFERRER,
  };

  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesViewCounterTest,
                           NotActiveInitially);
  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesViewCounterTest,
                           NotActiveWithBadData);
  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesViewCounterTest,
                           NotActiveOptedOut);
  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesViewCounterTest,
                           IsActiveOptedIn);
  FRIEND_TEST_ALL_PREFIXES(NTPSponsoredImagesViewCounterTest,
                           ActiveInitiallyOptedIn);

  void OnPreferenceChanged();

  // KeyedService
  void Shutdown() override;

  // NTPSponsoredImagesService::Observer
  void OnReferralImagesUpdated(NTPReferralImagesData* data) override;

  // NTPSponsoredImagesService::Observer
  void OnSponsoredImagesUpdated(NTPSponsoredImagesData* data) override;

  void ResetViewCounterModelByDataUpdated(int background_images_count);

  // APIs for sponsored wallpaper.
  void ResetNotificationState();
  bool IsSponsoredWallpaperOptedIn() const;
  // Do we have a sponsored wallpaper to show and has the user
  // opted-in to showing it at some time.
  bool IsSponsoredWallpaperActive() const;
  // Should we show the sponsored wallpaper right now, in addition
  // to the result from `IsSponsoredWallpaperActive()`.
  bool ShouldShowSponsoredWallpaper() const;
  // Gets the current data for branded wallpaper, if there
  // is a wallpaper active. Does not consider user opt-in
  // status, or consider whether the wallpaper should be shown.
  NTPSponsoredImagesData* GetCurrentSponsoredWallpaperData() const;

  // APIs for referral wallpaper.
  bool ShouldShowReferralWallpaper() const;
  bool IsReferralWallpaperOptedIn() const;
  bool IsReferralWallpaperActive() const;
  NTPReferralImagesData* GetCurrentReferralWallpaperData() const;

  NTPReferralImagesService* referral_service_ = nullptr;  // not owned
  NTPSponsoredImagesService* sponsored_service_ = nullptr;  // not owned
  PrefService* prefs_ = nullptr;  // not owned
  bool is_supported_locale_ = false;
  bool show_background_image_enabled_ = true;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<ViewCounterModel> model_;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_SERVICE_H_
