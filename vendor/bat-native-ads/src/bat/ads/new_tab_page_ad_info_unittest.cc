/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_info.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kNewTabPageAdInfoAsJson[] = R"(
  {
    "type": "new_tab_page_ad",
    "placement_id": "a337efac-34b8-4b85-8fb4-6bd97d33d16c",
    "creative_instance_id": "9ae0a1df-e249-4cb1-93b9-7a2e727d9dfd",
    "creative_set_id": "aea63f45-5295-498a-9b1b-dae1609b0cf7",
    "campaign_id": "2809e72c-0162-4a63-9751-f63ca1aa4571",
    "advertiser_id": "f8eb8aa5-e05b-462f-a249-57757765f00b",
    "segment": "Segment",
    "company_name": "Company Name",
    "image_url": "https://brave.com/image_1",
    "alt": "Alt",
    "wallpapers": [
      {
        "image_url": "https://brave.com/wallpaper/image_1",
        "focal_point": {
          "x": 1,
          "y": 2
        }
      },
      {
        "image_url": "https://brave.com/wallpaper/image_2",
        "focal_point": {
          "x": 3,
          "y": 4
        }
      }
    ],
    "target_url": "https://brave.com/"
  }
)";

NewTabPageAdInfo BuildNewTabPageAd() {
  NewTabPageAdInfo ad;

  ad.type = AdType::kNewTabPageAd;
  ad.placement_id = "a337efac-34b8-4b85-8fb4-6bd97d33d16c";
  ad.creative_instance_id = "9ae0a1df-e249-4cb1-93b9-7a2e727d9dfd";
  ad.creative_set_id = "aea63f45-5295-498a-9b1b-dae1609b0cf7";
  ad.campaign_id = "2809e72c-0162-4a63-9751-f63ca1aa4571";
  ad.advertiser_id = "f8eb8aa5-e05b-462f-a249-57757765f00b";
  ad.segment = "Segment";
  ad.company_name = "Company Name";
  ad.image_url = GURL("https://brave.com/image_1");
  ad.alt = "Alt";
  ad.target_url = GURL("https://brave.com/");

  NewTabPageAdWallpaperInfo wallpaper_1;
  wallpaper_1.image_url = GURL("https://brave.com/wallpaper/image_1");
  wallpaper_1.focal_point.x = 1;
  wallpaper_1.focal_point.y = 2;
  ad.wallpapers.push_back(wallpaper_1);

  NewTabPageAdWallpaperInfo wallpaper_2;
  wallpaper_2.image_url = GURL("https://brave.com/wallpaper/image_2");
  wallpaper_2.focal_point.x = 3;
  wallpaper_2.focal_point.y = 4;
  ad.wallpapers.push_back(wallpaper_2);

  return ad;
}

}  // namespace

class BatAdsNewTabPageAdInfoTest : public UnitTestBase {
 protected:
  BatAdsNewTabPageAdInfoTest() = default;

  ~BatAdsNewTabPageAdInfoTest() override = default;
};

TEST_F(BatAdsNewTabPageAdInfoTest, Deserialize) {
  // Arrange
  const base::Value value = base::test::ParseJson(kNewTabPageAdInfoAsJson);
  const base::Value::Dict* dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  NewTabPageAdInfo ad;
  ad.FromValue(*dict);

  // Assert
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();
  EXPECT_EQ(expected_ad, ad);
}

TEST_F(BatAdsNewTabPageAdInfoTest, Serialize) {
  // Arrange
  const NewTabPageAdInfo ad = BuildNewTabPageAd();

  // Act
  const base::Value::Dict value = ad.ToValue();

  // Assert
  const base::Value expected_value =
      base::test::ParseJson(kNewTabPageAdInfoAsJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
