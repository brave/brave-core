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
#include "brave/components/ntp_background_images/browser/ntp_sponsored_image_source.h"
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
        /*component_update_service=*/nullptr, &pref_service_);
    sponsored_image_source_ = std::make_unique<NTPSponsoredImageSource>(
        background_images_service_.get());
    background_images_source_ = std::make_unique<NTPBackgroundImagesSource>(
        background_images_service_.get());

    pref_service_.SetDict(prefs::kNewTabPageCachedSuperReferralComponentInfo,
                          base::Value::Dict());
  }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPSponsoredImageSource> sponsored_image_source_;
  std::unique_ptr<NTPBackgroundImagesSource> background_images_source_;
};

TEST_F(NTPBackgroundImagesSourceTest, SponsoredImagesTest) {
  const std::string test_json_string_referral = R"JSON(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl":
"6690ad47-d0af-4dbb-a2dd-c7a678b2b83b/background.jpg", "focalPoint": { "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl":
"6690ad47-d0af-4dbb-a2dd-c7a678b2b83b/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ]
    })JSON";
  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/false, test_json_string_referral);
  EXPECT_FALSE(sponsored_image_source_->AllowCaching());
  EXPECT_TRUE(
      sponsored_image_source_->IsValidPath("sponsored-images/button.png"));
  EXPECT_TRUE(
      sponsored_image_source_->IsValidPath("sponsored-images/background.jpg"));
  EXPECT_FALSE(sponsored_image_source_->IsValidPath("super-duper/brave.png"));
  EXPECT_FALSE(
      sponsored_image_source_->IsValidPath("sponsored-images/abcd.png"));

  EXPECT_EQ("image/jpeg", sponsored_image_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.jpg")));
  EXPECT_EQ("image/jpeg", sponsored_image_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.jpeg")));
  EXPECT_EQ("image/webp", sponsored_image_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.webp")));
  EXPECT_EQ("image/png", sponsored_image_source_->GetMimeType(
                             GURL("brave://test/wallpaper-0.png")));
  EXPECT_EQ("image/avif", sponsored_image_source_->GetMimeType(
                              GURL("brave://test/wallpaper-0.avif")));
  EXPECT_THAT(sponsored_image_source_->GetMimeType(GURL("brave://test/")),
              ::testing::IsEmpty());
}

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

#if !BUILDFLAG(IS_LINUX)
TEST_F(NTPBackgroundImagesSourceTest, BasicSuperReferralDataTest) {
  // Valid super referral component json data.
  const std::string test_json_string_referral = R"JSON(
    {
      "schemaVersion": 2,
      "themeName": "Technikke",
      "campaigns": [
        {
          "version": 1,
          "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
          "creativeSets": [
            {
              "creativeSetId": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b",
              "creatives": [
                {
                  "creativeInstanceId": "30244a36-561a-48f0-8d7a-780e9035c57a",
                  "companyName": "Image NTT Creative",
                  "alt": "Some content",
                  "targetUrl": "https://basicattentiontoken.org",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b/background.jpg",
                    "focalPoint": {
                      "x": 25,
                      "y": 50
                    },
                    "button": {
                      "image": {
                        "relativeUrl": "6690ad47-d0af-4dbb-a2dd-c7a678b2b83b/button.png"
                      }
                    }
                  }
                }
              ]
            }
          ]
        }
      ],
      "topSites": [
        {
          "name": "Brave",
          "destinationUrl": "https://brave.com/",
          "backgroundColor": "#e22919",
          "iconUrl": "brave.png"
        },
        {
          "name": "Wiki",
          "destinationUrl": "https://wikipedia.org/",
          "backgroundColor": "#e22919",
          "iconUrl": "wikipedia.png"
        },
        {
          "name": "BAT",
          "destinationUrl": "https://basicattentiontoken.org/",
          "backgroundColor": "#e22919",
          "iconUrl": "bat.png"
        }
      ]
    })JSON";
  background_images_service_->OnGetSponsoredComponentJsonData(
      /*is_super_referral=*/true, test_json_string_referral);
  EXPECT_FALSE(sponsored_image_source_->AllowCaching());
  EXPECT_TRUE(sponsored_image_source_->IsValidPath("super-referral/bat.png"));
  EXPECT_TRUE(
      sponsored_image_source_->IsValidPath("super-referral/button.png"));
  EXPECT_TRUE(
      sponsored_image_source_->IsValidPath("super-referral/background.jpg"));
  EXPECT_TRUE(sponsored_image_source_->IsValidPath("super-referral/brave.png"));
  EXPECT_FALSE(
      sponsored_image_source_->IsValidPath("sponsored-images/button.png"));
  EXPECT_FALSE(sponsored_image_source_->IsValidPath("super-duper/brave.png"));
  EXPECT_FALSE(sponsored_image_source_->IsValidPath("super-referral/abcd.png"));
}
#endif

}  // namespace ntp_background_images
