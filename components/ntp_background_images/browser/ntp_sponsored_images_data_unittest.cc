/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include "base/dcheck_is_on.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

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

constexpr char kTestImageCampaignWithWallpaperRelativeUrlReferencingParent[] =
    R"JSON(
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
                  "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
                  "wallpaper": {
                    "type": "image",
                    "relativeUrl": "../background.jpg",
                    "button": {
                      "image": {
                        "relativeUrl": "39d78863-327d-4b64-9952-cd0e5e330eb6/button.png"
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

constexpr char kTestRichMediaCampaign[] = R"JSON(
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
                    "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
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
      })JSON";

constexpr char
    kTestRichMediaCampaignWithWallpaperRelativeUrlReferencingParent[] = R"JSON(
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
                  "creativeInstanceId": "39d78863-327d-4b64-9952-cd0e5e330eb6",
                  "wallpaper": {
                    "type": "richMedia",
                    "relativeUrl": "../index.html"
                  }
                }
              ]
            }
          ]
        }
      ]
    })JSON";

}  // namespace

TEST(NTPSponsoredImagesDataTest, EmptyJson) {
  base::DictValue dict;
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsFalse());
}

TEST(NTPSponsoredImagesDataTest, EmptyCampaigns) {
  base::DictValue dict = base::test::ParseJsonDict(kTestEmptyCampaigns);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsFalse());
}

TEST(NTPSponsoredImagesDataTest, ParseSponsoredImageCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  ASSERT_EQ(data.campaigns.size(), 1U);
  const auto& campaign = data.campaigns[0];
  EXPECT_EQ(campaign.campaign_id, "65933e82-6b21-440b-9956-c0f675ca7435");
  ASSERT_EQ(campaign.creatives.size(), 1U);
  const auto& creative = campaign.creatives[0];
  EXPECT_EQ(creative.wallpaper_type, WallpaperType::kImage);
  EXPECT_EQ(creative.creative_instance_id,
            "30244a36-561a-48f0-8d7a-780e9035c57a");
  EXPECT_EQ(creative.url,
            GURL("chrome://branded-wallpaper/"
                 "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg"));
  EXPECT_EQ(creative.file_path,
            installed_dir.AppendASCII("30244a36-561a-48f0-8d7a-780e9035c57a")
                .AppendASCII("background-1.jpg"));
  EXPECT_EQ(creative.focal_point, gfx::Point(25, 50));

  EXPECT_EQ(creative.logo.company_name, "Image NTT Creative");
  EXPECT_EQ(creative.logo.alt_text, "Some content");
  EXPECT_EQ(creative.logo.destination_url, "https://basicattentiontoken.org");
  EXPECT_EQ(creative.logo.image_file,
            installed_dir.AppendASCII("30244a36-561a-48f0-8d7a-780e9035c57a")
                .AppendASCII("button-1.png"));
  EXPECT_EQ(creative.logo.image_url,
            "chrome://branded-wallpaper/30244a36-561a-48f0-8d7a-780e9035c57a/"
            "button-1.png");
}

TEST(NTPSponsoredImagesDataTest,
     GetCreativeByInstanceIdFromSponsoredImagesCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData data(dict, installed_dir);
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  EXPECT_EQ(
      data.GetCreativeByInstanceId("30244a36-561a-48f0-8d7a-780e9035c57a"),
      &data.campaigns[0].creatives[0]);
  EXPECT_EQ(
      data.GetCreativeByInstanceId("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"),
      nullptr);
}

TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsRemovesImageCreativeIfRelativeUrlReferencesParent) {
  base::DictValue dict = base::test::ParseJsonDict(
      kTestImageCampaignWithWallpaperRelativeUrlReferencingParent);
  FilterCampaigns(dict, base::FilePath(FILE_PATH_LITERAL("installed_dir")));
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::IsEmpty()));
}

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsRemovesImageCreativeIfWallpaperFileIsMissing) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  FilterCampaigns(dict, installed_dir.GetPath());
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::IsEmpty()));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsKeepsImageCreativeIfCreativeFilesExist) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  const base::FilePath creative_dir = installed_dir.GetPath().AppendASCII(
      "30244a36-561a-48f0-8d7a-780e9035c57a");
  ASSERT_TRUE(base::CreateDirectory(creative_dir));
  ASSERT_TRUE(
      base::WriteFile(creative_dir.AppendASCII("background-1.jpg"), ""));
  ASSERT_TRUE(base::WriteFile(creative_dir.AppendASCII("button-1.png"), ""));

  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  FilterCampaigns(dict, installed_dir.GetPath());
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::SizeIs(1)));
}

TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsRemovesRichMediaCreativeIfRelativeUrlReferencesParent) {
  base::DictValue dict = base::test::ParseJsonDict(
      kTestRichMediaCampaignWithWallpaperRelativeUrlReferencingParent);
  FilterCampaigns(dict, base::FilePath(FILE_PATH_LITERAL("installed_dir")));
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::IsEmpty()));
}

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsRemovesRichMediaCreativeIfFileIsMissing) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(kTestRichMediaCampaign);
  FilterCampaigns(dict, installed_dir.GetPath());
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::IsEmpty()));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

TEST(NTPSponsoredImagesDataTest,
     FilterCampaignsKeepsRichMediaCreativeIfFileExists) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  const base::FilePath creative_dir = installed_dir.GetPath().AppendASCII(
      "39d78863-327d-4b64-9952-cd0e5e330eb6");
  ASSERT_TRUE(base::CreateDirectory(creative_dir));
  ASSERT_TRUE(base::WriteFile(creative_dir.AppendASCII("index.html"), ""));

  base::DictValue dict = base::test::ParseJsonDict(kTestRichMediaCampaign);
  FilterCampaigns(dict, installed_dir.GetPath());
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::SizeIs(1)));
}

}  // namespace ntp_background_images
