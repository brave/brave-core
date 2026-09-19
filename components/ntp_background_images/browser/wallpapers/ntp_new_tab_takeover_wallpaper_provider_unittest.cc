/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_new_tab_takeover_wallpaper_provider.h"

#include <memory>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Only needed by the DUMP_WILL_BE_NOTREACHED()-guarded test below.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
#include "url/gurl.h"
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

namespace ntp_background_images {

namespace {

// Only used by the DUMP_WILL_BE_NOTREACHED()-guarded test below.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
constexpr char kCreativeInstanceId[] = "c0d61af3-3b85-4af4-a3cc-cf1b3dd40e70";
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

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
                  "companyName": "foo",
                  "alt": "bar",
                  "targetUrl": "https://brave.com",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "foo/background-1.jpg",
                    "focalPoint": { "x": 25, "y": 50 },
                    "button": {
                      "image": {
                        "relativeUrl": "foo/button-1.png"
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

// Only used by the DUMP_WILL_BE_NOTREACHED()-guarded test below.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
brave_ads::mojom::NewTabPageAdInfoPtr BuildNewTabPageAd() {
  brave_ads::mojom::NewTabPageAdInfoPtr ad =
      brave_ads::mojom::NewTabPageAdInfo::New();
  ad->creative_instance_id = kCreativeInstanceId;
  ad->creative_set_id = "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b";
  ad->campaign_id = "fb7ee174-5430-4fb9-8e97-29bf14e8d828";
  ad->target_url = GURL("https://brave.com");
  ad->company_name = "foo";
  ad->alt = "bar";
  return ad;
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace

class NTPNewTabTakeoverWallpaperProviderTest : public testing::Test {
 public:
  NTPNewTabTakeoverWallpaperProviderTest() = default;

  void SetUp() override {
    brave_ads::RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefs(prefs_.registry());
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());

    host_content_settings_map_ = new HostContentSettingsMap(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);

    background_images_service_ =
        std::make_unique<FakeNTPBackgroundImagesService>(
            /*variations_service=*/nullptr,
            /*component_updater_service=*/nullptr, &prefs_);

    view_counter_model_ = std::make_unique<ViewCounterModel>(&prefs_);
  }

  void TearDown() override {
    host_content_settings_map_->ShutdownOnUIThread();
  }

  std::unique_ptr<NTPNewTabTakeoverWallpaperProvider> CreateWallpaperProvider(
      bool is_supported_locale = true) {
    return std::make_unique<NTPNewTabTakeoverWallpaperProvider>(
        *background_images_service_, *view_counter_model_,
        *host_content_settings_map_, prefs_, ads_service_mock_,
        is_supported_locale);
  }

  void MakeEligible() {
    background_images_service_->OnGetSponsoredComponentJsonData(
        kSponsoredImageCampaignsJson);
    prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, true);
    prefs_.SetBoolean(prefs::kNewTabPageShowBackgroundImage, true);
    view_counter_model_->SetCampaignsTotalNewTabTakeoverCreativeCount({1});
    view_counter_model_->RegisterPageView();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<FakeNTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<ViewCounterModel> view_counter_model_;
  brave_ads::AdsServiceMock ads_service_mock_;
};

TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NotEligibleWithoutSponsoredData) {
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_FALSE(wallpaper_provider->IsEligible());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest, NotEligibleWhenOptedOut) {
  background_images_service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageCampaignsJson);
  prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, false);
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_FALSE(wallpaper_provider->IsEligible());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NotEligibleWhenBackgroundImagesOptedOut) {
  background_images_service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageCampaignsJson);
  prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(prefs::kNewTabPageShowBackgroundImage, false);
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_FALSE(wallpaper_provider->IsEligible());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest, EligibleWhenOptedInWithData) {
  MakeEligible();
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_TRUE(wallpaper_provider->IsEligible());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NotEligibleForUnsupportedLocale) {
  background_images_service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageCampaignsJson);
  prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(prefs::kNewTabPageShowBackgroundImage, true);
  auto wallpaper_provider =
      CreateWallpaperProvider(/*is_supported_locale=*/false);

  EXPECT_FALSE(wallpaper_provider->IsEligible());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NoWallpaperWithoutSponsoredData) {
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_CALL(ads_service_mock_, MaybeServeNewTabPageAd).Times(0);
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());

  EXPECT_FALSE(test_future.Take());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NoWallpaperOrAdRequestWhenOptedOut) {
  background_images_service_->OnGetSponsoredComponentJsonData(
      kSponsoredImageCampaignsJson);
  prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, false);
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_CALL(ads_service_mock_, MaybeServeNewTabPageAd).Times(0);
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());

  EXPECT_FALSE(test_future.Take());
}

TEST_F(NTPNewTabTakeoverWallpaperProviderTest, NoWallpaperWhenNoAdIsServed) {
  MakeEligible();
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_CALL(ads_service_mock_, MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(nullptr));
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());

  EXPECT_FALSE(test_future.Take());
}

// DUMP_WILL_BE_NOTREACHED() fires when the creative file is missing, which
// terminates the test in non-official DCHECK builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPNewTabTakeoverWallpaperProviderTest,
       NoWallpaperWhenCreativeFileIsMissing) {
  MakeEligible();
  auto wallpaper_provider = CreateWallpaperProvider();

  EXPECT_CALL(ads_service_mock_, MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(BuildNewTabPageAd()));
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  wallpaper_provider->MaybeGetWallpaper(test_future.GetCallback());

  // The fake service's installed directory does not exist on disk, so the
  // creative file existence check fails even though the ad matched.
  EXPECT_FALSE(test_future.Take());
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace ntp_background_images
