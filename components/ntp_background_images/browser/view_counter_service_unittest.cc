/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"
#endif

namespace ntp_background_images {

std::unique_ptr<NTPSponsoredImagesData> GetDemoBrandedWallpaper(
    bool super_referral) {
  auto demo = std::make_unique<NTPSponsoredImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  Logo demo_logo;
  demo_logo.alt_text = "Technikke: For music lovers.";
  demo_logo.company_name = "Technikke";
  demo_logo.destination_url = "https://brave.com";

  Campaign demo_campaign;
  demo_campaign.backgrounds = {
      {base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")),
       {3988, 2049},
       demo_logo},
      {base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")),
       {5233, 3464},
       demo_logo},
      {base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")), {0, 0}, demo_logo},
  };
  demo->campaigns.push_back(demo_campaign);

  if (super_referral) {
    demo->theme_name = "Technikke";
    demo->top_sites = {
      { "Brave", "https://brave.com", "brave.png",
        base::FilePath(FILE_PATH_LITERAL("brave.png")) },
     { "BAT", "https://basicattentiontoken.org/", "bat.png",
        base::FilePath(FILE_PATH_LITERAL("bat.png")) },
    };
  }

  return demo;
}

std::unique_ptr<NTPBackgroundImagesData> GetDemoBackgroundWallpaper() {
  auto demo = std::make_unique<NTPBackgroundImagesData>();
  demo->backgrounds = {
      {base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")), "Brave",
       "https://brave.com/"},
  };

  return demo;
}

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
class TestDelegate : public NTPCustomBackgroundImagesService::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() override = default;

  // Delegate overrides:
  bool IsCustomBackgroundEnabled() override { return enabled_; }
  base::FilePath GetCustomBackgroundImageLocalFilePath() override {
    return base::FilePath();
  }

  bool enabled_ = false;
};
#endif

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
    NTPBackgroundImagesService::RegisterLocalStatePrefs(local_registry);
    ViewCounterService::RegisterLocalStatePrefs(local_registry);

    service_ = std::make_unique<NTPBackgroundImagesService>(nullptr,
                                                            &local_pref_);
#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
    auto delegate = std::make_unique<TestDelegate>();
    delegate_ = delegate.get();
    custom_bi_service_ =
        std::make_unique<NTPCustomBackgroundImagesService>(std::move(delegate));
    view_counter_ = std::make_unique<ViewCounterService>(
        service_.get(), custom_bi_service_.get(), nullptr, prefs(),
        &local_pref_, true);
#else
    view_counter_ = std::make_unique<ViewCounterService>(
        service_.get(), nullptr, nullptr, prefs(), &local_pref_, true);
#endif

    // Set referral service is properly initialized sr component is set.
    local_pref_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
    local_pref_.Set(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                    base::Value(base::Value::Type::DICTIONARY));
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

  void EnableNTPBGImagesPref(bool enable) {
    prefs()->SetBoolean(prefs::kNewTabPageShowBackgroundImage, enable);
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment;
  TestingPrefServiceSimple local_pref_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ViewCounterService> view_counter_;

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  std::unique_ptr<NTPCustomBackgroundImagesService> custom_bi_service_;
  raw_ptr<TestDelegate> delegate_ = nullptr;
#endif

  std::unique_ptr<NTPBackgroundImagesService> service_;
};

TEST_F(NTPBackgroundImagesViewCounterTest, SINotActiveInitially) {
  // By default, data is bad and SI wallpaper is not active.
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, BINotActiveInitially) {
  // By default, data is bad and BI wallpaper is not active.
  EXPECT_FALSE(view_counter_->IsBackgroundWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, SINotActiveWithBadData) {
  // Set some bad data explicitly.
  service_->si_images_data_.reset(new NTPSponsoredImagesData);
  service_->sr_images_data_.reset(new NTPSponsoredImagesData);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, BINotActiveWithBadData) {
  // Set some bad data explicitly.
  service_->bi_images_data_.reset(new NTPBackgroundImagesData);
  EXPECT_FALSE(view_counter_->IsBackgroundWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, NotActiveOptedOut) {
  // Even with good data, wallpaper should not be active if user pref is off.
  service_->si_images_data_ = GetDemoBrandedWallpaper(false);
  EnableSIPref(false);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());

  service_->sr_images_data_ = GetDemoBrandedWallpaper(true);
  EnableSRPref(false);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest,
       ActiveOptedInWithNTPBackgoundOption) {
  EnableNTPBGImagesPref(false);
  service_->sr_images_data_ = GetDemoBrandedWallpaper(true);

  // Even with bg images turned off, SR wallpaper should be active.
  EnableSRPref(true);
#if BUILDFLAG(IS_LINUX)
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
#else
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
#endif

  EnableSRPref(false);
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest,
       BINotActiveWithNTPBackgoundOptionOptedOut) {
  EnableNTPBGImagesPref(false);
  service_->bi_images_data_ = GetDemoBackgroundWallpaper();
#if BUILDFLAG(IS_ANDROID)
  // On android, |kNewTabPageShowBackgroundImage| prefs is not used for
  // controlling bg option. So view counter can give data.
  EXPECT_TRUE(view_counter_->IsBackgroundWallpaperActive());
#else
  EXPECT_FALSE(view_counter_->IsBackgroundWallpaperActive());
#endif
}

// Branded wallpaper is active if one of them is available.
TEST_F(NTPBackgroundImagesViewCounterTest, IsActiveOptedIn) {
  service_->si_images_data_ = GetDemoBrandedWallpaper(false);
  EnableSIPref(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  service_->sr_images_data_ = GetDemoBrandedWallpaper(true);
  EnableSRPref(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  // Active if SI is possible.
  EnableSRPref(false);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  // Active if SR is only opted in.
  EnableSIPref(false);
  EnableSRPref(true);
#if BUILDFLAG(IS_LINUX)
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
#else
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
#endif
}

TEST_F(NTPBackgroundImagesViewCounterTest, PrefsWithModelTest) {
  auto& model = view_counter_->model_;
  EXPECT_TRUE(model.show_wallpaper_);
  EXPECT_TRUE(model.show_branded_wallpaper_);
  EXPECT_FALSE(model.always_show_branded_wallpaper_);

  EnableSRPref(true);
  EXPECT_FALSE(model.always_show_branded_wallpaper_);

  EnableSIPref(false);
  EXPECT_FALSE(model.show_branded_wallpaper_);

  EnableNTPBGImagesPref(false);
  EXPECT_FALSE(model.show_wallpaper_);
}

TEST_F(NTPBackgroundImagesViewCounterTest, ActiveInitiallyOptedIn) {
  // Sanity check that the default is still to be opted-in.
  // If this gets manually changed, then this test should be manually changed
  // too.
  service_->si_images_data_ = GetDemoBrandedWallpaper(false);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());

  service_->sr_images_data_ = GetDemoBrandedWallpaper(true);
  EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
}

#if !BUILDFLAG(IS_LINUX)
// Super referral feature is disabled on linux.
TEST_F(NTPBackgroundImagesViewCounterTest, ModelTest) {
  service_->sr_images_data_ = GetDemoBrandedWallpaper(true);
  service_->si_images_data_ = GetDemoBrandedWallpaper(false);
  view_counter_->OnUpdated(service_->sr_images_data_.get());
  EXPECT_TRUE(view_counter_->model_.always_show_branded_wallpaper_);

  // Initial count is not changed because branded wallpaper is always
  // visible in SR mode.
  int expected_count = ViewCounterModel::kInitialCountToBrandedWallpaper;
  view_counter_->RegisterPageView();
  view_counter_->RegisterPageView();
  EXPECT_EQ(expected_count, view_counter_->model_.count_to_branded_wallpaper_);

  service_->sr_images_data_.reset(new NTPSponsoredImagesData);
  view_counter_->OnSuperReferralEnded();
  EXPECT_FALSE(view_counter_->model_.always_show_branded_wallpaper_);
  EXPECT_EQ(expected_count, view_counter_->model_.count_to_branded_wallpaper_);

  view_counter_->RegisterPageView();
  expected_count--;
  EXPECT_EQ(expected_count, view_counter_->model_.count_to_branded_wallpaper_);
}
#endif

TEST_F(NTPBackgroundImagesViewCounterTest, GetCurrentWallpaperTest) {
  service_->bi_images_data_ = GetDemoBackgroundWallpaper();
  EXPECT_TRUE(view_counter_->IsBackgroundWallpaperActive());
  base::Value background = view_counter_->GetCurrentWallpaper();
  std::string* bg_url = background.FindStringKey(kWallpaperImageURLKey);
  EXPECT_EQ("chrome://background-wallpaper/wallpaper1.jpg", *bg_url);

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  // Enable custom background.
  delegate_->enabled_ = true;
  background = view_counter_->GetCurrentWallpaper();
  bg_url = background.FindStringKey(kWallpaperImageURLKey);
  EXPECT_EQ("chrome://custom-wallpaper/background.jpg", *bg_url);

  // Disable custom background.
  delegate_->enabled_ = false;
  background = view_counter_->GetCurrentWallpaper();
  bg_url = background.FindStringKey(kWallpaperImageURLKey);
  EXPECT_EQ("chrome://background-wallpaper/wallpaper1.jpg", *bg_url);
#endif
}
}  // namespace ntp_background_images
