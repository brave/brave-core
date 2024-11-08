/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"
#endif

using testing::_;
using testing::Return;

namespace ntp_background_images {

namespace {

constexpr char kPlacementdId[] = "326eb47b-467b-46ab-ac1b-5f5de780b344";
constexpr char kCampaignId[] = "fb7ee174-5430-4fb9-8e97-29bf14e8d828";
constexpr char kFirstCreativeInstanceId[] =
    "ab257ca5-2bbc-4288-9c06-ce1d5d796343";

constexpr char kAltText[] = "Technikke: For music lovers.";
constexpr char kCompanyName[] = "Technikke";
constexpr char kLogoImageFile[] = "logo_image.png";
constexpr char kLogoImageUrl[] = "https://static.bave.com/logos/logo_image.png";
constexpr char kDestinationUrl[] = "https://brave.com";
constexpr char kCreativeInstanceId[] = "c0d61af3-3b85-4af4-a3cc-cf1b3dd40e70";
constexpr char kSponsoredImageFile[] = "wallpaper2.jpg";
constexpr char kSponsoredImageUrl[] =
    "https://static.bave.com/image/wallpaper2.jpg";
constexpr int kSponsoredImageFocalPointX = 5233;
constexpr int kSponsoredImageFocalPointY = 3464;

}  // namespace

std::unique_ptr<NTPSponsoredImagesData> GetDemoBrandedWallpaper(
    bool super_referral) {
  auto demo = std::make_unique<NTPSponsoredImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  Logo demo_logo;
  demo_logo.alt_text = kAltText;
  demo_logo.company_name = kCompanyName;
  demo_logo.destination_url = kDestinationUrl;
  demo_logo.image_file = base::FilePath::FromUTF8Unsafe(kLogoImageFile);

  Campaign demo_campaign;
  demo_campaign.campaign_id = kCampaignId;
  demo_campaign.backgrounds = {
      {base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")),
       {3988, 2049},
       demo_logo,
       kFirstCreativeInstanceId},
      {base::FilePath::FromUTF8Unsafe(kSponsoredImageFile),
       {kSponsoredImageFocalPointX, kSponsoredImageFocalPointY},
       demo_logo,
       kCreativeInstanceId},
      {base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")),
       {0, 0},
       demo_logo,
       "1744602b-253b-47b2-909b-f9b248a6b681"},
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
class TestDelegate : public BraveNTPCustomBackgroundService::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() override = default;

  // Delegate overrides:
  bool IsCustomImageBackgroundEnabled() const override {
    return image_enabled_;
  }
  base::FilePath GetCustomBackgroundImageLocalFilePath(
      const GURL& url) const override {
    return base::FilePath();
  }
  GURL GetCustomBackgroundImageURL() const override {
    return GURL(std::string(kCustomWallpaperURL) + "foo.jpg");
  }

  bool IsColorBackgroundEnabled() const override { return color_enabled_; }
  std::string GetColor() const override { return "#ff0000"; }
  bool ShouldUseRandomValue() const override { return false; }
  bool HasPreferredBraveBackground() const override { return false; }
  base::Value::Dict GetPreferredBraveBackground() const override { return {}; }

  bool image_enabled_ = false;
  bool color_enabled_ = false;
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
    brave_rewards::RegisterProfilePrefs(registry);
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
        std::make_unique<BraveNTPCustomBackgroundService>(std::move(delegate));
    view_counter_ = std::make_unique<ViewCounterService>(
        service_.get(), custom_bi_service_.get(), &ads_service_mock_, prefs(),
        &local_pref_,
        // don't need to test p3a, so passing nullptr
        std::unique_ptr<NTPP3AHelper>(),
        /* is_supported_locale */ true);
#else
    view_counter_ = std::make_unique<ViewCounterService>(
        service_.get(), nullptr, &ads_service_mock_, prefs(), &local_pref_,
        std::unique_ptr<NTPP3AHelper>(), true);
#endif

    // Set referral service is properly initialized sr component is set.
    local_pref_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
    local_pref_.SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                        base::Value::Dict());
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

  void InitBackgroundAndSponsoredImageWallpapers() {
    service_->si_images_data_ = GetDemoBrandedWallpaper(false);
    EnableSIPref(true);
    EnableNTPBGImagesPref(true);
    service_->bi_images_data_ = GetDemoBackgroundWallpaper();

    EXPECT_TRUE(view_counter_->IsBrandedWallpaperActive());
    EXPECT_TRUE(view_counter_->IsBackgroundWallpaperActive());
  }

  brave_ads::NewTabPageAdInfo CreateNewTabPageAdInfo() {
    brave_ads::NewTabPageAdInfo ad_info;
    ad_info.placement_id = kPlacementdId;
    ad_info.campaign_id = kCampaignId;
    ad_info.creative_instance_id = kCreativeInstanceId;
    ad_info.company_name = kCompanyName;
    ad_info.alt = kAltText;
    ad_info.image_url = GURL(kLogoImageUrl);
    ad_info.target_url = GURL(kDestinationUrl);
    brave_ads::NewTabPageAdWallpaperInfo wallpaper_info;
    wallpaper_info.image_url = GURL(kSponsoredImageUrl);
    wallpaper_info.focal_point.x = kSponsoredImageFocalPointX;
    wallpaper_info.focal_point.y = kSponsoredImageFocalPointY;
    ad_info.wallpapers.push_back(wallpaper_info);
    return ad_info;
  }

  int GetInitialCountToBrandedWallpaper() const {
    return features::kInitialCountToBrandedWallpaper.Get() - 1;
  }

  std::optional<base::Value::Dict> TryGetFirstSponsoredImageWallpaper() {
    // Loading initial count times.
    for (int i = 0; i < GetInitialCountToBrandedWallpaper(); ++i) {
      auto wallpaper = view_counter_->GetCurrentWallpaperForDisplay();
      EXPECT_TRUE(wallpaper->FindBool(ntp_background_images::kIsBackgroundKey)
                      .value_or(false));
      view_counter_->RegisterPageView();
    }

    return view_counter_->GetCurrentWallpaperForDisplay();
  }

  bool AdInfoMatchesSponsoredImage(const brave_ads::NewTabPageAdInfo& ad_info,
                                   size_t campaign_index,
                                   size_t background_index) {
    return service_->si_images_data_->AdInfoMatchesSponsoredImage(
        ad_info, campaign_index, background_index);
  }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment;
  TestingPrefServiceSimple local_pref_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<NTPBackgroundImagesService> service_;

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  std::unique_ptr<BraveNTPCustomBackgroundService> custom_bi_service_;
  raw_ptr<TestDelegate> delegate_ = nullptr;
#endif

  std::unique_ptr<ViewCounterService> view_counter_;
  brave_ads::AdsServiceMock ads_service_mock_{nullptr};
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
  service_->si_images_data_ = std::make_unique<NTPSponsoredImagesData>();
  service_->sr_images_data_ = std::make_unique<NTPSponsoredImagesData>();
  EXPECT_FALSE(view_counter_->IsBrandedWallpaperActive());
}

TEST_F(NTPBackgroundImagesViewCounterTest, BINotActiveWithBadData) {
  // Set some bad data explicitly.
  service_->bi_images_data_ = std::make_unique<NTPBackgroundImagesData>();
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

  EXPECT_EQ(features::kInitialCountToBrandedWallpaper.Get() - 1,
            model.show_branded_wallpaper_);
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
  int expected_count = GetInitialCountToBrandedWallpaper();
  view_counter_->RegisterPageView();
  view_counter_->RegisterPageView();
  EXPECT_EQ(expected_count, view_counter_->model_.count_to_branded_wallpaper_);

  service_->sr_images_data_ = std::make_unique<NTPSponsoredImagesData>();
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
  auto background = view_counter_->GetCurrentWallpaper();
  std::string* bg_url = background->FindString(kWallpaperImageURLKey);
  EXPECT_EQ("chrome://background-wallpaper/wallpaper1.jpg", *bg_url);

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  // Enable custom image background.
  delegate_->image_enabled_ = true;
  background = view_counter_->GetCurrentWallpaper();
  bg_url = background->FindString(kWallpaperImageURLKey);
  EXPECT_TRUE(bg_url->starts_with(kCustomWallpaperURL))
      << "actual url " << *bg_url;

  // Disable custom image background.
  delegate_->image_enabled_ = false;
  background = view_counter_->GetCurrentWallpaper();
  bg_url = background->FindString(kWallpaperImageURLKey);
  EXPECT_EQ("chrome://background-wallpaper/wallpaper1.jpg", *bg_url);

  // Enable color background
  delegate_->color_enabled_ = true;
  background = view_counter_->GetCurrentWallpaper();
  EXPECT_FALSE(background->FindString(kWallpaperImageURLKey));
  EXPECT_EQ(delegate_->GetColor(), *background->FindString(kWallpaperColorKey));
#endif
}

TEST_F(NTPBackgroundImagesViewCounterTest,
       GetSponsoredImageWallpaperAdsServiceDisabled) {
  InitBackgroundAndSponsoredImageWallpapers();

  prefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAdForDisplay())
      .Times(0);
  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd()).Times(0);

  auto si_wallpaper = TryGetFirstSponsoredImageWallpaper();
  EXPECT_FALSE(si_wallpaper->FindBool(ntp_background_images::kIsBackgroundKey)
                   .value_or(true));
  ASSERT_TRUE(
      si_wallpaper->FindString(ntp_background_images::kCreativeInstanceIDKey));
  EXPECT_TRUE(
      si_wallpaper->FindString(ntp_background_images::kCreativeInstanceIDKey));
  ASSERT_TRUE(si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey));
  EXPECT_FALSE(si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey)
                   ->empty());
}

TEST_F(NTPBackgroundImagesViewCounterTest, SponsoredImageAdFrequencyCapped) {
  InitBackgroundAndSponsoredImageWallpapers();

  prefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAdForDisplay())
      .WillOnce(Return(std::nullopt));
  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd())
      .Times(GetInitialCountToBrandedWallpaper());
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd(_, _)).Times(0);

  auto si_wallpaper = TryGetFirstSponsoredImageWallpaper();
  EXPECT_TRUE(si_wallpaper);
  EXPECT_TRUE(si_wallpaper->FindBool(ntp_background_images::kIsBackgroundKey)
                  .value_or(false));
  EXPECT_FALSE(
      si_wallpaper->FindString(ntp_background_images::kCreativeInstanceIDKey));
  EXPECT_FALSE(
      si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey));
}

TEST_F(NTPBackgroundImagesViewCounterTest, SponsoredImageAdServed) {
  InitBackgroundAndSponsoredImageWallpapers();

  brave_ads::NewTabPageAdInfo ad_info = CreateNewTabPageAdInfo();
  EXPECT_TRUE(AdInfoMatchesSponsoredImage(ad_info, 0, 1));

  prefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAdForDisplay())
      .WillOnce(Return(ad_info));
  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd())
      .Times(GetInitialCountToBrandedWallpaper());
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd(_, _)).Times(0);

  auto si_wallpaper = TryGetFirstSponsoredImageWallpaper();
  EXPECT_FALSE(si_wallpaper->FindBool(ntp_background_images::kIsBackgroundKey)
                   .value_or(true));
  ASSERT_TRUE(
      si_wallpaper->FindString(ntp_background_images::kCreativeInstanceIDKey));
  EXPECT_EQ(kCreativeInstanceId,
            *(si_wallpaper->FindString(
                ntp_background_images::kCreativeInstanceIDKey)));
  ASSERT_TRUE(si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey));
  EXPECT_EQ(
      ad_info.placement_id,
      *(si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey)));
}

TEST_F(NTPBackgroundImagesViewCounterTest, WrongSponsoredImageAdServed) {
  InitBackgroundAndSponsoredImageWallpapers();

  brave_ads::NewTabPageAdInfo ad_info = CreateNewTabPageAdInfo();
  ad_info.creative_instance_id = "wrong_creative_instance_id";
  EXPECT_FALSE(AdInfoMatchesSponsoredImage(ad_info, 0, 1));

  prefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAdForDisplay())
      .WillOnce(Return(ad_info));
  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd())
      .Times(GetInitialCountToBrandedWallpaper());
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd(_, _));

  auto si_wallpaper = TryGetFirstSponsoredImageWallpaper();
  EXPECT_TRUE(si_wallpaper);
  EXPECT_TRUE(si_wallpaper->FindBool(ntp_background_images::kIsBackgroundKey)
                  .value_or(false));
  EXPECT_FALSE(
      si_wallpaper->FindString(ntp_background_images::kCreativeInstanceIDKey));
  EXPECT_FALSE(
      si_wallpaper->FindString(ntp_background_images::kWallpaperIDKey));
}

}  // namespace ntp_background_images
