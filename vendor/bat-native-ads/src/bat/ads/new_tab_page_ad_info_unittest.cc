/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"
#include "base/json/json_reader.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kSampleNewTabPageAdInfoJson[] = R"(
  {
    "type": "new_tab_page_ad",
    "placement_id": "a337efac-34b8-4b85-8fb4-6bd97d33d16c",
    "creative_instance_id": "9ae0a1df-e249-4cb1-93b9-7a2e727d9dfd",
    "creative_set_id": "aea63f45-5295-498a-9b1b-dae1609b0cf7",
    "campaign_id": "2809e72c-0162-4a63-9751-f63ca1aa4571",
    "advertiser_id": "f8eb8aa5-e05b-462f-a249-57757765f00b",
    "segment": "test-test",
    "company_name": "test company",
    "image_url": "https://static.bave.com/testing_image_logo",
    "alt": "testing alt",
    "wallpapers": [
      {
        "image_url": "https://static.bave.com/testing_wallpaper1",
        "focal_point": {
          "x": 1,
          "y": 2
        }
      },
      {
        "image_url": "https://static.bave.com/testing_wallpaper2",
        "focal_point": {
          "x": 3,
          "y": 4
        }
      }
    ],
    "target_url": "https://brave.com/"
  }
)";

NewTabPageAdInfo GetSampleNewTabPageAdInfo() {
  NewTabPageAdInfo ad_info;
  ad_info.type = AdType::kNewTabPageAd;
  ad_info.placement_id = "a337efac-34b8-4b85-8fb4-6bd97d33d16c";
  ad_info.creative_instance_id = "9ae0a1df-e249-4cb1-93b9-7a2e727d9dfd";
  ad_info.creative_set_id = "aea63f45-5295-498a-9b1b-dae1609b0cf7";
  ad_info.campaign_id = "2809e72c-0162-4a63-9751-f63ca1aa4571";
  ad_info.advertiser_id = "f8eb8aa5-e05b-462f-a249-57757765f00b";
  ad_info.segment = "test-test";
  ad_info.company_name = "test company";
  ad_info.image_url = GURL("https://static.bave.com/testing_image_logo");
  ad_info.alt = "testing alt";
  ad_info.target_url = GURL("https://brave.com/");

  NewTabPageAdWallpaperInfo wallpaper_info;
  wallpaper_info.image_url = GURL("https://static.bave.com/testing_wallpaper1");
  wallpaper_info.focal_point.x = 1;
  wallpaper_info.focal_point.y = 2;
  ad_info.wallpapers.push_back(wallpaper_info);

  wallpaper_info.image_url = GURL("https://static.bave.com/testing_wallpaper2");
  wallpaper_info.focal_point.x = 3;
  wallpaper_info.focal_point.y = 4;
  ad_info.wallpapers.push_back(wallpaper_info);

  return ad_info;
}

}  // namespace

class BatAdsNewTabPageAdInfoTest : public UnitTestBase {
 protected:
  BatAdsNewTabPageAdInfoTest() = default;

  ~BatAdsNewTabPageAdInfoTest() override = default;
};

TEST_F(BatAdsNewTabPageAdInfoTest, DeserializeNewTabPageAdInfo) {
  NewTabPageAdInfo ad_info;
  ASSERT_TRUE(ad_info.FromJson(kSampleNewTabPageAdInfoJson));

  NewTabPageAdInfo expected_ad_info = GetSampleNewTabPageAdInfo();
  EXPECT_EQ(expected_ad_info, ad_info);
}

TEST_F(BatAdsNewTabPageAdInfoTest, SerializeNewTabPageAdInfo) {
  NewTabPageAdInfo ad_info = GetSampleNewTabPageAdInfo();

  auto document = base::JSONReader::Read(kSampleNewTabPageAdInfoJson);
  ASSERT_TRUE(document);
  ASSERT_TRUE(document->is_dict());

  EXPECT_EQ(document->GetDict(), ad_info.ToValue());
}

}  // namespace ads
