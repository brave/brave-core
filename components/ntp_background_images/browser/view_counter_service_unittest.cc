/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

std::unique_ptr<NTPBackgroundImagesData> GetDemoWallpaper(bool super_referral) {
  auto demo = std::make_unique<NTPBackgroundImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  demo->backgrounds = {
      { base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")), { 3988, 2049 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")), { 5233, 3464 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")), {  0, 0 } },
  };
  demo->logo_alt_text = "Technikke: For music lovers.";
  demo->logo_company_name = "Technikke";
  demo->logo_destination_url = "https://brave.com";

  if (super_referral) {
    demo->top_sites = {
      { "Brave", "https://brave.com", "brave.png",
        base::FilePath(FILE_PATH_LITERAL("brave.png")) },
     { "BAT", "https://basicattentiontoken.org/", "bat.png",
        base::FilePath(FILE_PATH_LITERAL("bat.png")) },
    };
  }

  return demo;
}

class NTPBackgroundImagesViewCounterTest : public testing::Test {
 public:
  NTPBackgroundImagesViewCounterTest() = default;
  ~NTPBackgroundImagesViewCounterTest() override = default;

  void SetUp() override {
    // Need ntp_sponsored_images prefs
    auto* registry = prefs()->registry();
    ViewCounterService::RegisterProfilePrefs(registry);
    auto* local_registry = local_pref_.registry();
    brave::RegisterPrefsForBraveReferralsService(local_registry);
    RegisterLocalStatePrefs(local_registry);

    service_ = std::make_unique<NTPBackgroundImagesService>(
        nullptr,
        &local_pref_,
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>());
    view_counter_ = std::make_unique<ViewCounterService>(
        service_.get(), prefs(), true);
  }

  void EnableSIPref(bool enable) {
    prefs()->SetBoolean(
        prefs::kNewTabPageShowSponsoredImagesBackgroundImage, enable);
  }

  void EnableSRPref(bool enable) {
    prefs()->SetInteger(
        prefs::kNewTabPageSuperReferralThemesOption,
        enable ? ViewCounterService::SUPER_REFERRAL
               : ViewCounterService::DEFAULT);
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment;
  TestingPrefServiceSimple local_pref_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ViewCounterService> view_counter_;
  std::unique_ptr<NTPBackgroundImagesService> service_;
};

TEST_F(NTPBackgroundImagesViewCounterTest, NotActiveInitially) {
  // By default, data is bad and wallpaper is not active.
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, NotActiveWithBadData) {
  // Set some bad data explicitly.
  NTPBackgroundImagesData* badData = new NTPBackgroundImagesData;
  service_->images_data_.reset(badData);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, NotActiveOptedOut) {
  // Even with good data, wallpaper should not be active if user pref is off.
  service_->images_data_ = GetDemoWallpaper(false);
  EnableSIPref(false);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());

  service_->images_data_ = GetDemoWallpaper(true);
  EnableSRPref(false);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

// Branded wallpaper is active if one of them is available.
TEST_F(NTPBackgroundImagesViewCounterTest, IsActiveOptedIn) {
  service_->images_data_ = GetDemoWallpaper(false);
  EnableSIPref(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  service_->images_data_ = GetDemoWallpaper(true);
  EnableSRPref(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, ActiveInitiallyOptedIn) {
  // Sanity check that the default is still to be opted-in.
  // If this gets manually changed, then this test should be manually changed
  // too.
  service_->images_data_ = GetDemoWallpaper(false);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  service_->images_data_ = GetDemoWallpaper(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
}

}  // namespace ntp_background_images
