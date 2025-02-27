/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include "base/files/file_path.h"
#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace ntp_background_images {

constexpr char kTestEmptyCampaigns[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
      ]
    })";

constexpr char kTestSponsoredImagesCampaign[] = R"(
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

constexpr char kTestSponsoredRichMediaCampaign[] = R"(
    {
      "schemaVersion": 2,
      "campaigns": [
        {
          "version": 1,
          "campaignId": "c27a3fae-ee9e-48a2-b3a7-f4675744e6ec",
          "creativeSets": [
            {
              "creativeSetId": "a245e3b9-2df4-47f5-aaab-67b61c528b6f",
              "creatives": [
                {
                  "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
                  "alt": "Some more rich content",
                  "companyName": "Another Rich Media NTT Creative",
                  "targetUrl": "https://basicattentiontoken.org",
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

TEST(NTPSponsoredImagesDataTest, EmptyJson) {
  base::Value::Dict dict;
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsFalse());
}

TEST(NTPSponsoredImagesDataTest, EmptyCampaigns) {
  base::Value::Dict dict = base::test::ParseJsonDict(kTestEmptyCampaigns);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsFalse());
}

TEST(NTPSponsoredImagesDataTest, ParseSponsoredImageCampaign) {
  base::Value::Dict dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  ASSERT_EQ(data.campaigns.size(), 1u);
  const auto& campaign = data.campaigns[0];
  EXPECT_EQ(campaign.campaign_id, "65933e82-6b21-440b-9956-c0f675ca7435");
  ASSERT_EQ(campaign.creatives.size(), 1u);
  const auto& creative = campaign.creatives[0];
  EXPECT_EQ(creative.wallpaper_type, WallpaperType::kImage);
  EXPECT_EQ(creative.creative_instance_id,
            "30244a36-561a-48f0-8d7a-780e9035c57a");
  EXPECT_EQ(creative.url,
            GURL("chrome://branded-wallpaper/sponsored-images/"
                 "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg"));
  EXPECT_EQ(creative.file_path,
            installed_dir.AppendASCII(
                "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg"));
  EXPECT_EQ(creative.focal_point, gfx::Point(25, 50));
  EXPECT_EQ(creative.background_color, "");
  EXPECT_EQ(creative.condition_matchers, brave_ads::ConditionMatcherMap());
  EXPECT_EQ(creative.viewbox, gfx::Rect{});

  EXPECT_EQ(creative.logo.company_name, "Image NTT Creative");
  EXPECT_EQ(creative.logo.alt_text, "Some content");
  EXPECT_EQ(creative.logo.destination_url, "https://basicattentiontoken.org");
  EXPECT_EQ(creative.logo.image_file,
            installed_dir.AppendASCII(
                "30244a36-561a-48f0-8d7a-780e9035c57a/button-1.png"));
  EXPECT_EQ(creative.logo.image_url,
            "chrome://branded-wallpaper/sponsored-images/"
            "30244a36-561a-48f0-8d7a-780e9035c57a/button-1.png");
}

TEST(NTPSponsoredImagesDataTest, ParseSponsoredRichMediaCampaign) {
  base::Value::Dict dict =
      base::test::ParseJsonDict(kTestSponsoredRichMediaCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  ASSERT_EQ(data.campaigns.size(), 1u);
  const auto& campaign = data.campaigns[0];
  EXPECT_EQ(campaign.campaign_id, "c27a3fae-ee9e-48a2-b3a7-f4675744e6ec");
  ASSERT_EQ(campaign.creatives.size(), 1u);
  const auto& creative = campaign.creatives[0];
  EXPECT_EQ(creative.wallpaper_type, WallpaperType::kRichMedia);
  EXPECT_EQ(creative.creative_instance_id,
            "39d78863-327d-4b64-9952-cd0e5e330eb6");
  EXPECT_EQ(creative.url,
            GURL("chrome-untrusted://new-tab-takeover/"
                 "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"));
  EXPECT_EQ(creative.file_path,
            installed_dir.AppendASCII(
                "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"));
  EXPECT_EQ(creative.focal_point, gfx::Point(0, 0));
  EXPECT_EQ(creative.background_color, "");
  EXPECT_EQ(creative.condition_matchers, brave_ads::ConditionMatcherMap());
  EXPECT_EQ(creative.viewbox, std::nullopt);

  EXPECT_EQ(creative.logo.company_name, "Another Rich Media NTT Creative");
  EXPECT_EQ(creative.logo.alt_text, "Some more rich content");
  EXPECT_EQ(creative.logo.destination_url, "https://basicattentiontoken.org");
  EXPECT_THAT(creative.logo.image_file.empty(), testing::IsTrue());
  EXPECT_THAT(creative.logo.image_url, testing::IsEmpty());
}

}  // namespace ntp_background_images
