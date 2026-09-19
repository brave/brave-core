/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include <string_view>

#include "base/check_deref.h"
#include "base/dcheck_is_on.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr char kImageCreativeInstanceId[] =
    "30244a36-561a-48f0-8d7a-780e9035c57a";
constexpr char kRichMediaCreativeInstanceId[] =
    "39d78863-327d-4b64-9952-cd0e5e330eb6";

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

#if BUILDFLAG(ENABLE_BRAVE_ADS)
void SetCreativeTargetUrls(base::DictValue& dict, std::string_view target_url) {
  for (base::Value& campaign : CHECK_DEREF(dict.FindList("campaigns"))) {
    for (base::Value& creative_set :
         CHECK_DEREF(campaign.GetDict().FindList("creativeSets"))) {
      for (base::Value& creative :
           CHECK_DEREF(creative_set.GetDict().FindList("creatives"))) {
        creative.GetDict().Set("targetUrl", target_url);
      }
    }
  }
}
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

}  // namespace

TEST(NTPSponsoredImagesDataTest, EmptyJson) {
  base::DictValue dict;
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData sponsored_images_data(dict, installed_dir);
  EXPECT_FALSE(sponsored_images_data.IsValid());
}

TEST(NTPSponsoredImagesDataTest, EmptyCampaigns) {
  base::DictValue dict = base::test::ParseJsonDict(kTestEmptyCampaigns);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData sponsored_images_data(dict, installed_dir);
  EXPECT_FALSE(sponsored_images_data.IsValid());
}

TEST(NTPSponsoredImagesDataTest, ParseSponsoredImageCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData sponsored_images_data(dict, installed_dir);
  EXPECT_TRUE(sponsored_images_data.IsValid());

  EXPECT_THAT(
      sponsored_images_data.campaigns,
      testing::ElementsAre(testing::FieldsAre(
          /*campaign_id=*/"65933e82-6b21-440b-9956-c0f675ca7435",
          /*creatives=*/
          testing::ElementsAre(testing::FieldsAre(
              WallpaperType::kImage,
              GURL("chrome://branded-wallpaper/"
                   "30244a36-561a-48f0-8d7a-780e9035c57a/background-1.jpg"),
              /*file_path=*/
              installed_dir.AppendASCII(kImageCreativeInstanceId)
                  .AppendASCII("background-1.jpg"),
              /*focal_point=*/gfx::Point(25, 50), kImageCreativeInstanceId,
              brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
              /*logo=*/
              testing::FieldsAre(
                  /*image_file=*/
                  installed_dir.AppendASCII(kImageCreativeInstanceId)
                      .AppendASCII("button-1.png"),
                  /*image_url=*/
                  "chrome://branded-wallpaper/"
                  "30244a36-561a-48f0-8d7a-780e9035c57a/button-1.png",
                  /*alt_text=*/"Some content",
                  /*destination_url=*/"https://basicattentiontoken.org",
                  /*company_name=*/"Image NTT Creative"))))));
}

TEST(NTPSponsoredImagesDataTest,
     GetCreativeByInstanceIdReturnsCreativeForKnownInstanceId) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData sponsored_images_data(dict, installed_dir);
  EXPECT_TRUE(sponsored_images_data.IsValid());

  const Creative* const creative =
      sponsored_images_data.GetCreativeByInstanceId(kImageCreativeInstanceId);
  ASSERT_TRUE(creative);
  EXPECT_EQ(creative->creative_instance_id, kImageCreativeInstanceId);
}

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST(NTPSponsoredImagesDataTest,
     GetCreativeByInstanceIdReturnsNullForUnknownInstanceId) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_images_data"));
  NTPSponsoredImagesData sponsored_images_data(dict, installed_dir);
  EXPECT_TRUE(sponsored_images_data.IsValid());

  EXPECT_FALSE(sponsored_images_data.GetCreativeByInstanceId(
      "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

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

  const base::FilePath creative_dir =
      installed_dir.GetPath().AppendASCII(kImageCreativeInstanceId);
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

  const base::FilePath creative_dir =
      installed_dir.GetPath().AppendASCII(kRichMediaCreativeInstanceId);
  ASSERT_TRUE(base::CreateDirectory(creative_dir));
  ASSERT_TRUE(base::WriteFile(creative_dir.AppendASCII("index.html"), ""));

  base::DictValue dict = base::test::ParseJsonDict(kTestRichMediaCampaign);
  FilterCampaigns(dict, installed_dir.GetPath());
  EXPECT_THAT(dict.FindList("campaigns"), testing::Pointee(testing::SizeIs(1)));
}

#if BUILDFLAG(ENABLE_BRAVE_ADS)
TEST(NTPSponsoredImagesDataTest, RejectsCreativeWithHttpTargetUrl) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  SetCreativeTargetUrls(dict, "http://basicattentiontoken.org");
  NTPSponsoredImagesData sponsored_images_data(
      dict, base::FilePath(FILE_PATH_LITERAL("ntp_sponsored_images_data")));
  EXPECT_THAT(sponsored_images_data.campaigns, testing::IsEmpty());
}

TEST(NTPSponsoredImagesDataTest, RejectsCreativeWithJavascriptTargetUrl) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  SetCreativeTargetUrls(dict, "javascript:alert(1)");
  NTPSponsoredImagesData sponsored_images_data(
      dict, base::FilePath(FILE_PATH_LITERAL("ntp_sponsored_images_data")));
  EXPECT_THAT(sponsored_images_data.campaigns, testing::IsEmpty());
}

TEST(NTPSponsoredImagesDataTest, RejectsCreativeWithMalformedTargetUrl) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredImagesCampaign);
  SetCreativeTargetUrls(dict, "MALFORMED_TARGET_URL");
  NTPSponsoredImagesData sponsored_images_data(
      dict, base::FilePath(FILE_PATH_LITERAL("ntp_sponsored_images_data")));
  EXPECT_THAT(sponsored_images_data.campaigns, testing::IsEmpty());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

}  // namespace ntp_background_images
