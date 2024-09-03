/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_source.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPBackgroundImagesSourceTest : public testing::Test {
 public:
  NTPBackgroundImagesSourceTest() = default;

  void SetUp() override {
    auto* registry = local_pref_.registry();
    NTPBackgroundImagesService::RegisterLocalStatePrefs(registry);
    brave::RegisterPrefsForBraveReferralsService(registry);
    service_ =
        std::make_unique<NTPBackgroundImagesService>(nullptr, &local_pref_);
    source_ = std::make_unique<NTPSponsoredImagesSource>(service_.get());
    bg_source_ = std::make_unique<NTPBackgroundImagesSource>(service_.get());
    local_pref_.SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                        base::Value::Dict());
  }

  base::test::SingleThreadTaskEnvironment task_environment;
  TestingPrefServiceSimple local_pref_;
  std::unique_ptr<NTPBackgroundImagesService> service_;
  std::unique_ptr<NTPSponsoredImagesSource> source_;
  std::unique_ptr<NTPBackgroundImagesSource> bg_source_;
};

TEST_F(NTPBackgroundImagesSourceTest, SponsoredImagesTest) {
  const std::string test_json_string_referral = R"(
      {
        "schemaVersion": 1,
        "logo": {
          "imageUrl": "logo.png",
          "alt": "Technikke: For music lovers",
          "companyName": "Technikke",
          "destinationUrl": "https://www.brave.com/?from-super-referreer-demo"
        },
        "wallpapers": [
          {
            "imageUrl": "background-1.jpg",
            "focalPoint": { "x": 3988, "y": 2049}
          },
          {
            "imageUrl": "background-2.jpg",
            "focalPoint": { "x": 5233, "y": 3464}
          },
          {
            "imageUrl": "background-3.jpg"
          }
        ]
      })";
  service_->OnGetSponsoredComponentJsonData(false, test_json_string_referral);
  EXPECT_FALSE(source_->AllowCaching());
  EXPECT_TRUE(source_->IsValidPath("sponsored-images/logo.png"));
  EXPECT_TRUE(source_->IsValidPath("sponsored-images/background-1.jpg"));
  EXPECT_TRUE(source_->IsValidPath("sponsored-images/background-2.jpg"));
  EXPECT_TRUE(source_->IsValidPath("sponsored-images/background-3.jpg"));
  EXPECT_FALSE(source_->IsValidPath("super-duper/brave.png"));
  EXPECT_FALSE(source_->IsValidPath("sponsored-images/abcd.png"));

  EXPECT_EQ("image/jpeg",
            source_->GetMimeType(GURL("brave://test/wallpaper-0.jpg")));
  EXPECT_EQ("image/jpeg",
            source_->GetMimeType(GURL("brave://test/wallpaper-0.jpeg")));
  EXPECT_EQ("image/webp",
            source_->GetMimeType(GURL("brave://test/wallpaper-0.webp")));
  EXPECT_EQ("image/png",
            source_->GetMimeType(GURL("brave://test/wallpaper-0.png")));
  EXPECT_EQ("image/avif",
            source_->GetMimeType(GURL("brave://test/wallpaper-0.avif")));
  EXPECT_EQ("", source_->GetMimeType(GURL("brave://test/")));
}

TEST_F(NTPBackgroundImagesSourceTest, BackgroundImagesFormatTest) {
  EXPECT_EQ("image/jpeg",
            bg_source_->GetMimeType(GURL("brave://test/wallpaper-0.jpg")));
  EXPECT_EQ("image/webp",
            bg_source_->GetMimeType(GURL("brave://test/wallpaper-0.webp")));
  EXPECT_EQ("image/png",
            bg_source_->GetMimeType(GURL("brave://test/wallpaper-0.png")));
  EXPECT_EQ("image/avif",
            bg_source_->GetMimeType(GURL("brave://test/wallpaper-0.avif")));
  EXPECT_EQ("", bg_source_->GetMimeType(GURL("brave://test/")));
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
  service_->OnGetComponentJsonData(test_json_string);
  EXPECT_TRUE(bg_source_->AllowCaching());
  EXPECT_EQ(0, bg_source_->GetWallpaperIndexFromPath("brave-bg-1.webp"));
  EXPECT_EQ(1, bg_source_->GetWallpaperIndexFromPath("brave-bg-2.webp"));
  EXPECT_EQ(2, bg_source_->GetWallpaperIndexFromPath("brave-bg-3.webp"));
  EXPECT_EQ(3, bg_source_->GetWallpaperIndexFromPath("brave-bg-4.webp"));
  EXPECT_EQ(-1, bg_source_->GetWallpaperIndexFromPath("wallpaper-3.jpg"));
}

#if !BUILDFLAG(IS_LINUX)
TEST_F(NTPBackgroundImagesSourceTest, BasicSuperReferralDataTest) {
  // Valid super referral component json data.
  const std::string test_json_string_referral = R"(
      {
        "schemaVersion": 1,
        "themeName": "Technikke",
        "logo": {
          "imageUrl": "logo.png",
          "alt": "Technikke: For music lovers",
          "companyName": "Technikke",
          "destinationUrl": "https://www.brave.com/?from-super-referreer-demo"
        },
        "wallpapers": [
          {
            "imageUrl": "background-1.jpg",
            "focalPoint": { "x": 3988, "y": 2049}
          },
          {
            "imageUrl": "background-2.jpg",
            "focalPoint": { "x": 5233, "y": 3464}
          },
          {
            "imageUrl": "background-3.jpg"
          }
        ],
        "topSites": [
          {
            "name": "Brave",
            "destinationUrl": "https://brave.com/",
            "iconUrl": "brave.png"
          },
          {
            "name": "Wiki",
            "destinationUrl": "https://wikipedia.org/",
            "iconUrl": "wikipedia.png"
          },
          {
            "name": "BAT",
            "destinationUrl": "https://basicattentiontoken.org/",
            "iconUrl": "bat.png"
          }
        ]
      })";
  service_->OnGetSponsoredComponentJsonData(true, test_json_string_referral);
  EXPECT_FALSE(source_->AllowCaching());
  EXPECT_TRUE(source_->IsValidPath("super-referral/bat.png"));
  EXPECT_TRUE(source_->IsValidPath("super-referral/logo.png"));
  EXPECT_TRUE(source_->IsValidPath("super-referral/background-1.jpg"));
  EXPECT_TRUE(source_->IsValidPath("super-referral/brave.png"));
  EXPECT_FALSE(source_->IsValidPath("sponsored-images/logo.png"));
  EXPECT_FALSE(source_->IsValidPath("super-duper/brave.png"));
  EXPECT_FALSE(source_->IsValidPath("super-referral/abcd.png"));
}
#endif

}  // namespace ntp_background_images
