/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPBackgroundImagesSourceTest : public testing::Test {
 public:
  NTPBackgroundImagesSourceTest() = default;

  void SetUp() override {
    PrefRegistrySimple* const pref_registry = pref_service_.registry();
    NTPBackgroundImagesService::RegisterLocalStatePrefs(pref_registry);
    brave::RegisterPrefsForBraveReferralsService(pref_registry);

    background_images_service_ = std::make_unique<NTPBackgroundImagesService>(
        /*variations_service*/ nullptr, /*component_update_service=*/nullptr,
        &pref_service_);
    background_images_source_ = std::make_unique<NTPBackgroundImagesSource>(
        background_images_service_.get());

    pref_service_.SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                          base::Value::Dict());
  }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPBackgroundImagesSource> background_images_source_;
};

TEST_F(NTPBackgroundImagesSourceTest, BackgroundImagesFormatTest) {
  EXPECT_EQ("image/jpeg", background_images_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.jpg")));
  EXPECT_EQ("image/webp", background_images_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.webp")));
  EXPECT_EQ("image/png", background_images_source_->GetMimeType(
                             GURL("brave://test/wallpaper-0.png")));
  EXPECT_EQ("image/avif", background_images_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.avif")));
  EXPECT_THAT(background_images_source_->GetMimeType(GURL("brave://test/")),
              ::testing::IsEmpty());
}

TEST_F(NTPBackgroundImagesSourceTest, BackgroundImagesTest) {
  const std::string test_json_string = R"(
      {
        "schemaVersion": 1,
        "images": [
          {
            "name": "background-1.jpg",
            "source": "brave-bg-1.webp",
            "author": "Brave software",
            "link": "https://www.brave.com/",
            "originalUrl": "Contributor sent the hi-res version",
            "license": "used with permission"
          },
          {
            "name": "background-2.jpg",
            "source": "brave-bg-2.webp",
            "author": "Brave software",
            "link": "https://www.brave.com/",
            "originalUrl": "Contributor sent the hi-res version",
            "license": "used with permission"
          },
          {
            "name": "background-3.jpg",
            "source": "brave-bg-3.webp",
            "author": "Brave software",
            "link": "https://www.brave.com/",
            "originalUrl": "Contributor sent the hi-res version",
            "license": "used with permission"
          },
          {
            "name": "background-4.jpg",
            "source": "brave-bg-4.webp",
            "author": "Brave software",
            "link": "https://www.brave.com/",
            "originalUrl": "Contributor sent the hi-res version",
            "license": "used with permission"
          }
        ]
      })";
  background_images_service_->OnGetComponentJsonData(test_json_string);
  EXPECT_TRUE(background_images_source_->AllowCaching());
  EXPECT_EQ(0, background_images_source_->GetWallpaperIndexFromPath(
                   "brave-bg-1.webp"));
  EXPECT_EQ(1, background_images_source_->GetWallpaperIndexFromPath(
                   "brave-bg-2.webp"));
  EXPECT_EQ(2, background_images_source_->GetWallpaperIndexFromPath(
                   "brave-bg-3.webp"));
  EXPECT_EQ(3, background_images_source_->GetWallpaperIndexFromPath(
                   "brave-bg-4.webp"));
  EXPECT_EQ(-1, background_images_source_->GetWallpaperIndexFromPath(
                    "wallpaper-3.jpg"));
}

}  // namespace ntp_background_images
