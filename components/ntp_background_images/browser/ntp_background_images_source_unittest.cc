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
#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/test/test_shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPBackgroundImagesSourceTest : public testing::Test {
 public:
  NTPBackgroundImagesSourceTest() {}

  void SetUp() override {
    auto* registry = local_pref_.registry();
    ntp_background_images::RegisterLocalStatePrefs(registry);
    brave::RegisterPrefsForBraveReferralsService(registry);
    service_.reset(new NTPBackgroundImagesService(
        nullptr, &local_pref_,
        base::MakeRefCounted<network::TestSharedURLLoaderFactory>()));
    source_.reset(new NTPBackgroundImagesSource(service_.get()));
  }

  base::test::SingleThreadTaskEnvironment task_environment;
  TestingPrefServiceSimple local_pref_;
  std::unique_ptr<NTPBackgroundImagesService> service_;
  std::unique_ptr<NTPBackgroundImagesSource> source_;
};

TEST_F(NTPBackgroundImagesSourceTest, BasicTest) {
  // Valid super referrer component json data.
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
  service_->OnGetComponentJsonData(test_json_string_referral);
  EXPECT_FALSE(source_->AllowCaching());
  EXPECT_TRUE(source_->IsTopSiteIconPath("bat.png"));
  EXPECT_FALSE(source_->IsTopSiteIconPath("logo.png"));
  EXPECT_TRUE(source_->IsWallpaperPath("wallpaper-1.jpg"));
  EXPECT_TRUE(source_->IsValidPath("brave.png"));
  EXPECT_FALSE(source_->IsValidPath("abcd.png"));
  EXPECT_EQ("image/png", source_->GetMimeType("logo.png"));
  EXPECT_EQ("image/jpg", source_->GetMimeType("wallpaper-2.jpg"));
  EXPECT_EQ(0, source_->GetWallpaperIndexFromPath("wallpaper-0.jpg"));
  EXPECT_EQ(-1, source_->GetWallpaperIndexFromPath("wallpaper-3.jpg"));
  EXPECT_EQ(1, source_->GetTopSiteIndexFromPath("wikipedia.png"));
  EXPECT_EQ(-1, source_->GetTopSiteIndexFromPath("abcd.png"));
}

}  // namespace ntp_background_images
