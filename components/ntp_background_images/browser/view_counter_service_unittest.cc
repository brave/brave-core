/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/view_counter_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "brave/components/ntp_background_images/common/view_counter_theme_option_type.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"
#endif  // BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)

namespace ntp_background_images {

namespace {

constexpr char kPlacementdId[] = "326eb47b-467b-46ab-ac1b-5f5de780b344";
constexpr char kCampaignId[] = "fb7ee174-5430-4fb9-8e97-29bf14e8d828";
constexpr char kCreativeInstanceId[] = "c0d61af3-3b85-4af4-a3cc-cf1b3dd40e70";
constexpr char kCompanyName[] = "Technikke";
constexpr char kAltText[] = "Technikke: For music lovers.";
constexpr char kTargetUrl[] = "https://brave.com";

constexpr char kSponsoredImageCampaignsJson[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "fb7ee174-5430-4fb9-8e97-29bf14e8d828",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "c0d61af3-3b85-4af4-a3cc-cf1b3dd40e70",
                  "companyName": "Technikke",
                  "alt": "Technikke: For music lovers.",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "30244a36-561a-48f0-8d7a-780e9035c57a/button-1.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

constexpr char kSponsoredRichMediaCampaignsJson[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "fb7ee174-5430-4fb9-8e97-29bf14e8d828",
          "creativeSets": [
            {
              "creativeSetId": "a245e3b9-2df4-47f5-aaab-67b61c528b6f",
              "creatives": [
                {
                  "creativeInstanceId": "c0d61af3-3b85-4af4-a3cc-cf1b3dd40e70",
                  "companyName": "Technikke",
                  "alt": "Technikke: For music lovers.",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "richMedia",
                    "relativeUrl": "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"
                  }
                }
              ]
            }
          ]
        }
      ]
    })";

}  // namespace

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
class BraveNTPCustomBackgroundServiceDelegateMock
    : public BraveNTPCustomBackgroundService::Delegate {
 public:
  BraveNTPCustomBackgroundServiceDelegateMock() = default;

  ~BraveNTPCustomBackgroundServiceDelegateMock() override = default;

  void EnableCustomImageBackground() {
    is_custom_image_background_enabled_ = true;
  }

  void DisableCustomImageBackground() {
    is_custom_image_background_enabled_ = false;
  }

  void EnableColorBackground() { is_color_background_enabled_ = true; }

  void DisableColorBackground() { is_color_background_enabled_ = false; }

  // Delegate:
  bool IsCustomImageBackgroundEnabled() const override {
    return is_custom_image_background_enabled_;
  }

  base::FilePath GetCustomBackgroundImageLocalFilePath(
      const GURL& /*url*/) const override {
    return {};
  }

  GURL GetCustomBackgroundImageURL() const override {
    return GURL(std::string(kCustomWallpaperURL) + "foo.jpg");
  }

  bool IsColorBackgroundEnabled() const override {
    return is_color_background_enabled_;
  }

  std::string GetColor() const override { return "#ff0000"; }

  bool ShouldUseRandomValue() const override { return false; }

  bool HasPreferredBraveBackground() const override { return false; }

  base::Value::Dict GetPreferredBraveBackground() const override { return {}; }

 private:
  bool is_custom_image_background_enabled_ = false;
  bool is_color_background_enabled_ = false;
};
#endif  // BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)

class ViewCounterServiceTest : public testing::Test {
 public:
  ViewCounterServiceTest() = default;

  ~ViewCounterServiceTest() override = default;

  void SetUp() override {
    brave_rewards::RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefs(prefs_.registry());
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());

    brave::RegisterPrefsForBraveReferralsService(local_state_.registry());
    NTPBackgroundImagesService::RegisterLocalStatePrefs(
        local_state_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    metrics::MetricsService::RegisterPrefs(local_state_.registry());

    local_state_.SetInt64(metrics::prefs::kInstallDate,
                          base::Time::Now().InSecondsFSinceUnixEpoch());

    host_content_settings_map_ = new HostContentSettingsMap(
        &prefs_, /* is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);

    background_images_service_ = std::make_unique<NTPBackgroundImagesService>(
        /*variations_service*/ nullptr, /*component_updater_service=*/nullptr,
        &local_state_);

    BraveNTPCustomBackgroundService* custom_background_service = nullptr;

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
    auto custom_background_service_delegate =
        std::make_unique<BraveNTPCustomBackgroundServiceDelegateMock>();
    custom_background_service_delegate_mock_ =
        custom_background_service_delegate.get();
    custom_background_service_ =
        std::make_unique<BraveNTPCustomBackgroundService>(
            std::move(custom_background_service_delegate));

    custom_background_service = custom_background_service_.get();
#endif  // BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)

    view_counter_service_ = std::make_unique<ViewCounterService>(
        host_content_settings_map_.get(), background_images_service_.get(),
        custom_background_service, &ads_service_mock_, &prefs_, &local_state_,
        std::unique_ptr<NTPP3AHelper>(),
        /*is_supported_locale=*/true);

    // Set referral service is properly initialized sr component is set.
    local_state_.SetBoolean(kReferralCheckedForPromoCodeFile, true);
    local_state_.SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                         base::Value::Dict());
  }

  void TearDown() override { host_content_settings_map_->ShutdownOnUIThread(); }

  void SetSponsoredImagesVisibility(bool should_show) {
    prefs_.SetBoolean(prefs::kNewTabPageShowSponsoredImagesBackgroundImage,
                      should_show);
  }

  void MockSponsoredImagesData(WallpaperType wallpaper_type,
                               bool super_referral) {
    auto images_data = std::make_unique<NTPSponsoredImagesData>();

    images_data->url_prefix = "chrome://branded-wallpaper/";

    Logo logo;
    logo.company_name = kCompanyName;
    logo.alt_text = kAltText;
    logo.image_file = base::FilePath(FILE_PATH_LITERAL("logo_image.png"));
    logo.destination_url = kTargetUrl;

    Campaign campaign;
    campaign.campaign_id = kCampaignId;
    campaign.creatives = {{wallpaper_type,
                           base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")),
                           {3988, 2049},
                           logo,
                           "ab257ca5-2bbc-4288-9c06-ce1d5d796343"},
                          {wallpaper_type,
                           base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")),
                           {5233, 3464},
                           logo,
                           kCreativeInstanceId},
                          {wallpaper_type,
                           base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")),
                           {0, 0},
                           logo,
                           "1744602b-253b-47b2-909b-f9b248a6b681"}};
    images_data->campaigns.push_back(campaign);

    if (super_referral) {
      images_data->theme_name = "Technikke";
      images_data->top_sites = {
          {"Brave", "https://brave.com", "brave.png",
           base::FilePath(FILE_PATH_LITERAL("brave.png"))},
          {"BAT", "https://basicattentiontoken.org/", "bat.png",
           base::FilePath(FILE_PATH_LITERAL("bat.png"))}};

      background_images_service_->super_referrals_images_data_ =
          std::move(images_data);
      return;
    }

    background_images_service_->sponsored_images_data_ = std::move(images_data);
  }

  void MockMalformedSponsoredImagesData() {
    background_images_service_->OnGetSponsoredComponentJsonData(
        /*is_super_referral=*/false, "MALFORMED JSON");
  }

  void MockBackgroundImagesData() {
    auto images_data = std::make_unique<NTPBackgroundImagesData>();
    images_data->backgrounds = {
        {base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")), "Brave",
         "https://brave.com/"}};

    background_images_service_->background_images_data_ =
        std::move(images_data);
  }

  void MockMalformedBackgroundImagesData() {
    background_images_service_->OnGetComponentJsonData("MALFORMED JSON");
  }

  void SetSuperReferralVisibility(bool should_show) {
    const ThemesOption themes_option =
        should_show ? ThemesOption::kSuperReferral : ThemesOption::kDefault;
    prefs_.SetInteger(prefs::kNewTabPageSuperReferralThemesOption,
                      static_cast<int>(themes_option));
  }

  void SetBackgroundImagesVisibility(bool should_show) {
    prefs_.SetBoolean(prefs::kNewTabPageShowBackgroundImage, should_show);
  }

  void MockBackgroundImagesService() {
    SetSponsoredImagesVisibility(true);
    MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
    EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

    SetBackgroundImagesVisibility(true);
    MockBackgroundImagesData();
    EXPECT_TRUE(view_counter_service_->CanShowBackgroundImages());
  }

  void InitBackgroundAndSponsoredRichMediaWallpapers() {
    SetSponsoredImagesVisibility(true);
    MockSponsoredImagesData(WallpaperType::kRichMedia,
                            /*super_referral=*/false);
    EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

    SetBackgroundImagesVisibility(true);
    MockBackgroundImagesData();
    EXPECT_TRUE(view_counter_service_->CanShowBackgroundImages());
  }

  brave_ads::NewTabPageAdInfo BuildNewTabPageAd() {
    brave_ads::NewTabPageAdInfo ad;
    ad.placement_id = kPlacementdId;
    ad.campaign_id = kCampaignId;
    ad.creative_instance_id = kCreativeInstanceId;
    ad.company_name = kCompanyName;
    ad.alt = kAltText;
    ad.target_url = GURL(kTargetUrl);
    return ad;
  }

  int GetInitialCountToBrandedWallpaper() const {
    return features::kInitialCountToBrandedWallpaper.Get() - 1;
  }

  std::optional<base::Value::Dict>
  CycleThroughPageViewsAndMaybeGetNewTabTakeoverWallpaper() {
    // Loading initial count times.
    for (int i = 0; i < GetInitialCountToBrandedWallpaper(); ++i) {
      const std::optional<base::Value::Dict> wallpaper =
          view_counter_service_->GetCurrentWallpaperForDisplay();
      EXPECT_TRUE(wallpaper);
      EXPECT_TRUE(wallpaper->FindBool(kIsBackgroundKey));

      view_counter_service_->RegisterPageView();
    }

    return view_counter_service_->GetCurrentWallpaperForDisplay();
  }

  void VerifyGetCurrentBrandedWallpaperExpectation() {
    EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd)
        .Times(GetInitialCountToBrandedWallpaper());
    const brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
    ON_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
        .WillByDefault(::testing::Return(ad));
    EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);

    const std::optional<base::Value::Dict> wallpaper =
        CycleThroughPageViewsAndMaybeGetNewTabTakeoverWallpaper();
    ASSERT_TRUE(wallpaper);

    const std::string* url = wallpaper->FindString(kWallpaperURLKey);
    ASSERT_TRUE(url);

    const std::string* placement_id = wallpaper->FindString(kWallpaperIDKey);
    ASSERT_TRUE(placement_id);

    const std::string* creative_instance_id =
        wallpaper->FindString(kCreativeInstanceIDKey);
    ASSERT_TRUE(creative_instance_id);

    const std::string* target_url =
        wallpaper->FindStringByDottedPath(kLogoDestinationURLPath);
    ASSERT_TRUE(target_url);

    base::MockCallback<GetCurrentBrandedWallpaperCallback> callback;
    EXPECT_CALL(callback, Run(::testing::Optional(GURL(*url)),
                              ::testing::Optional(*placement_id),
                              ::testing::Optional(*creative_instance_id),
                              /*should_metrics_fallback_to_p3a=*/false,
                              ::testing::Optional(GURL(*target_url))));
    view_counter_service_->GetCurrentBrandedWallpaper(callback.Get());
  }

  void VerifyDoNotGetCurrentBrandedWallpaperExpectation() {
    EXPECT_EQ(base::test::ParseJsonDict(R"JSON(
      {
        "author": "Brave",
        "isBackground": true,
        "link": "https://brave.com/",
        "random": true,
        "type": "brave",
        "wallpaperImagePath": "wallpaper1.jpg",
        "wallpaperImageUrl": "chrome://background-wallpaper/wallpaper1.jpg"
      })JSON"),
              CycleThroughPageViewsAndMaybeGetNewTabTakeoverWallpaper());

    base::MockCallback<GetCurrentBrandedWallpaperCallback> callback;
    EXPECT_CALL(
        callback,
        Run(::testing::Eq(std::nullopt), ::testing::Eq(std::nullopt),
            ::testing::Eq(std::nullopt), false, ::testing::Eq(std::nullopt)));
    view_counter_service_->GetCurrentBrandedWallpaper(callback.Get());
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;

  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;

  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  std::unique_ptr<BraveNTPCustomBackgroundService> custom_background_service_;
  raw_ptr<BraveNTPCustomBackgroundServiceDelegateMock>
      custom_background_service_delegate_mock_ = nullptr;
#endif  // BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)

  brave_ads::AdsServiceMock ads_service_mock_;

  std::unique_ptr<ViewCounterService> view_counter_service_;
};

TEST_F(ViewCounterServiceTest, CanShowSponsoredImages) {
  SetSponsoredImagesVisibility(true);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());
}

TEST_F(ViewCounterServiceTest, CannotShowSponsoredImagesIfOptedOut) {
  SetSponsoredImagesVisibility(false);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());

  SetSuperReferralVisibility(false);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/true);
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
}

TEST_F(ViewCounterServiceTest, CannotShowSponsoredImagesIfUninitialized) {
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
}

TEST_F(ViewCounterServiceTest, CannotShowSponsoredImagesIfMalformed) {
  SetSponsoredImagesVisibility(true);
  MockMalformedSponsoredImagesData();
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
}

TEST_F(ViewCounterServiceTest, CanShowBackgroundImages) {
  SetBackgroundImagesVisibility(true);
  MockBackgroundImagesData();
  EXPECT_TRUE(view_counter_service_->CanShowBackgroundImages());
}

TEST_F(ViewCounterServiceTest, CannotShowBackgroundImages) {
  EXPECT_FALSE(view_counter_service_->CanShowBackgroundImages());
}

TEST_F(ViewCounterServiceTest, CannotShowBackgroundImagesIfUninitialized) {
  EXPECT_FALSE(view_counter_service_->CanShowBackgroundImages());
}

TEST_F(ViewCounterServiceTest, CannotShowBackgroundImagesIfMalformed) {
  SetBackgroundImagesVisibility(true);
  MockMalformedBackgroundImagesData();
  EXPECT_FALSE(view_counter_service_->CanShowBackgroundImages());
}

TEST_F(ViewCounterServiceTest, ActiveOptedInWithNTPBackgoundOption) {
  SetBackgroundImagesVisibility(false);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/true);

  // Even with bg images turned off, SR wallpaper should be active.
  SetSuperReferralVisibility(true);
#if BUILDFLAG(IS_LINUX)
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
#else   // BUILDFLAG(IS_LINUX)
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());
#endif  // !BUILDFLAG(IS_LINUX)

  SetSuperReferralVisibility(false);
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
}

TEST_F(ViewCounterServiceTest, CannotShowBackgroundImagesIfOptedOut) {
  SetBackgroundImagesVisibility(false);
  MockBackgroundImagesData();

#if BUILDFLAG(IS_ANDROID)
  // On android, |kNewTabPageShowBackgroundImage| prefs is not used for
  // controlling bg option. So view counter can give data.
  EXPECT_TRUE(view_counter_service_->CanShowBackgroundImages());
#else   // BUILDFLAG(IS_ANDROID)
  EXPECT_FALSE(view_counter_service_->CanShowBackgroundImages());
#endif  // !BUILDFLAG(IS_ANDROID)
}

// New tab takeover wallpaper is active if one of them is available.
TEST_F(ViewCounterServiceTest, IsActiveOptedIn) {
  SetSponsoredImagesVisibility(true);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

  SetSuperReferralVisibility(true);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/true);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

  // Active if super referral is possible.
  SetSuperReferralVisibility(false);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

  // Active if SR is only opted in.
  SetSponsoredImagesVisibility(false);
  SetSuperReferralVisibility(true);
#if BUILDFLAG(IS_LINUX)
  EXPECT_FALSE(view_counter_service_->CanShowSponsoredImages());
#else   // BUILDFLAG(IS_LINUX)
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());
#endif  // !BUILDFLAG(IS_LINUX)
}

TEST_F(ViewCounterServiceTest, PrefsWithModelTest) {
  EXPECT_EQ(view_counter_service_->model_.show_branded_wallpaper_,
            features::kInitialCountToBrandedWallpaper.Get() - 1);
  EXPECT_TRUE(view_counter_service_->model_.show_wallpaper_);
  EXPECT_TRUE(view_counter_service_->model_.show_branded_wallpaper_);
  EXPECT_FALSE(view_counter_service_->model_.always_show_branded_wallpaper_);

  SetSuperReferralVisibility(true);
  EXPECT_FALSE(view_counter_service_->model_.always_show_branded_wallpaper_);

  SetSponsoredImagesVisibility(false);
  EXPECT_FALSE(view_counter_service_->model_.show_branded_wallpaper_);

  SetBackgroundImagesVisibility(false);
  EXPECT_FALSE(view_counter_service_->model_.show_wallpaper_);
}

TEST_F(ViewCounterServiceTest, ActiveInitiallyOptedIn) {
  // Sanity check that the default is still to be opted-in.
  // If this gets manually changed, then this test should be manually changed
  // too.
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());

  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/true);
  EXPECT_TRUE(view_counter_service_->CanShowSponsoredImages());
}

#if !BUILDFLAG(IS_LINUX)
// Super referral feature is disabled on linux.
TEST_F(ViewCounterServiceTest, ModelTest) {
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/true);
  MockSponsoredImagesData(WallpaperType::kImage, /*super_referral=*/false);
  view_counter_service_->OnSponsoredImagesDataDidUpdate(
      background_images_service_->super_referrals_images_data_.get());
  EXPECT_TRUE(view_counter_service_->model_.always_show_branded_wallpaper_);

  // Initial count is not changed because branded wallpaper is always
  // visible in SR mode.
  int expected_count = GetInitialCountToBrandedWallpaper();
  view_counter_service_->RegisterPageView();
  view_counter_service_->RegisterPageView();
  EXPECT_EQ(expected_count,
            view_counter_service_->model_.count_to_branded_wallpaper_);

  background_images_service_->super_referrals_images_data_ =
      std::make_unique<NTPSponsoredImagesData>();
  view_counter_service_->OnSuperReferralCampaignDidEnd();
  EXPECT_FALSE(view_counter_service_->model_.always_show_branded_wallpaper_);
  EXPECT_EQ(expected_count,
            view_counter_service_->model_.count_to_branded_wallpaper_);

  view_counter_service_->RegisterPageView();
  expected_count--;
  EXPECT_EQ(expected_count,
            view_counter_service_->model_.count_to_branded_wallpaper_);
}
#endif

TEST_F(ViewCounterServiceTest, GetCurrentWallpaper) {
  MockBackgroundImagesData();
  EXPECT_TRUE(view_counter_service_->CanShowBackgroundImages());

  EXPECT_EQ(base::test::ParseJsonDict(R"JSON(
      {
        "author": "Brave",
        "isBackground": true,
        "link": "https://brave.com/",
        "random": true,
        "type": "brave",
        "wallpaperImagePath": "wallpaper1.jpg",
        "wallpaperImageUrl": "chrome://background-wallpaper/wallpaper1.jpg"
      })JSON"),
            view_counter_service_->GetCurrentWallpaper());

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  custom_background_service_delegate_mock_->EnableCustomImageBackground();
  EXPECT_EQ(base::test::ParseJsonDict(R"JSON(
      {
        "isBackground": true,
        "random": false,
        "type": "image",
        "wallpaperImageUrl": "chrome://custom-wallpaper/foo.jpg"
      })JSON"),
            view_counter_service_->GetCurrentWallpaper());

  custom_background_service_delegate_mock_->DisableCustomImageBackground();
  EXPECT_EQ(base::test::ParseJsonDict(R"JSON(
      {
        "author": "Brave",
        "isBackground": true,
        "link": "https://brave.com/",
        "random": true,
        "type": "brave",
        "wallpaperImagePath": "wallpaper1.jpg",
        "wallpaperImageUrl": "chrome://background-wallpaper/wallpaper1.jpg"
      })JSON"),
            view_counter_service_->GetCurrentWallpaper());

  custom_background_service_delegate_mock_->EnableColorBackground();
  EXPECT_EQ(base::test::ParseJsonDict(R"JSON(
      {
        "isBackground": true,
        "random": false,
        "type": "color",
        "wallpaperColor": "#ff0000"
      })JSON"),
            view_counter_service_->GetCurrentWallpaper());
#endif  // BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
}

TEST_F(
    ViewCounterServiceTest,
    AllowNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToAllowed) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  host_content_settings_map_->SetDefaultContentSetting(
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_ALLOW);

  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/false, kSponsoredRichMediaCampaignsJson);
  ASSERT_TRUE(view_counter_service_->CanShowSponsoredImages());

  const brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
  ON_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
      .WillByDefault(::testing::Return(ad));
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  EXPECT_TRUE(view_counter_service_->GetCurrentBrandedWallpaper());
}

TEST_F(
    ViewCounterServiceTest,
    BlockNewTabTakeoverWithRichMediaIfJavaScriptContentSettingIsSetToBlocked) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  host_content_settings_map_->SetDefaultContentSetting(
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);

  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/false, kSponsoredRichMediaCampaignsJson);
  ASSERT_FALSE(view_counter_service_->CanShowSponsoredImages());

  const brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
  ON_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
      .WillByDefault(::testing::Return(ad));
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  EXPECT_FALSE(view_counter_service_->GetCurrentBrandedWallpaper());
}

TEST_F(ViewCounterServiceTest,
       AllowNewTabTakeoverWithImageIfJavaScriptContentSettingIsSetToAllowed) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  host_content_settings_map_->SetDefaultContentSetting(
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_ALLOW);

  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/false, kSponsoredImageCampaignsJson);
  ASSERT_TRUE(view_counter_service_->CanShowSponsoredImages());

  const brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
  ON_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
      .WillByDefault(::testing::Return(ad));
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  EXPECT_TRUE(view_counter_service_->GetCurrentBrandedWallpaper());
}

TEST_F(ViewCounterServiceTest,
       AllowNewTabTakeoverWithImageIfJavaScriptContentSettingIsSetToBlocked) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  host_content_settings_map_->SetDefaultContentSetting(
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);

  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/false, kSponsoredImageCampaignsJson);
  ASSERT_TRUE(view_counter_service_->CanShowSponsoredImages());

  const brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
  ON_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
      .WillByDefault(::testing::Return(ad));
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  EXPECT_TRUE(view_counter_service_->GetCurrentBrandedWallpaper());
}

TEST_F(ViewCounterServiceTest,
       DoNotGetNewTabTakeoverWallpaperForMissingCreativeInstanceId) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  MockBackgroundImagesService();

  brave_ads::NewTabPageAdInfo ad = BuildNewTabPageAd();
  ad.creative_instance_id = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";

  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd)
      .Times(GetInitialCountToBrandedWallpaper());
  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd)
      .WillOnce(::testing::Return(ad));
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd);
  VerifyDoNotGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       GetNewTabTakeoverWallpaperOutsideGracePeriodForNonRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);
  task_environment_.AdvanceClock(base::Minutes(1));

  VerifyGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       DoNotGetNewTabTakeoverWallpaperOnCuspOfGracePeriodForNonRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);
  task_environment_.AdvanceClock(base::Seconds(59));

  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  VerifyDoNotGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       DoNotGetNewTabTakeoverWallpaperWithinGracePeriodForNonRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);

  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  VerifyDoNotGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       GetNewTabTakeoverWallpaperOutsideGracePeriodForRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);
  task_environment_.AdvanceClock(base::Minutes(1));

  VerifyGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       DoNotGetNewTabTakeoverWallpaperOnCuspOfGracePeriodForRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);
  task_environment_.AdvanceClock(base::Seconds(59));

  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  VerifyDoNotGetCurrentBrandedWallpaperExpectation();
}

TEST_F(ViewCounterServiceTest,
       DoNotGetNewTabTakeoverWallpaperWithinGracePeriodForRewardsUser) {
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  MockBackgroundImagesService();
  background_images_service_->sponsored_images_data_->grace_period =
      base::Minutes(1);

  EXPECT_CALL(ads_service_mock_, PrefetchNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, MaybeGetPrefetchedNewTabPageAd).Times(0);
  EXPECT_CALL(ads_service_mock_, OnFailedToPrefetchNewTabPageAd).Times(0);
  VerifyDoNotGetCurrentBrandedWallpaperExpectation();
}

}  // namespace ntp_background_images
