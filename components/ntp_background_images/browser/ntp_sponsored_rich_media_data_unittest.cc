/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr char kRichMediaCreativeInstanceId[] =
    "39d78863-327d-4b64-9952-cd0e5e330eb6";

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

}  // namespace

class NTPSponsoredRichMediaImagesDataTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::CreateDirectory(
        temp_dir_.GetPath().AppendASCII(kRichMediaCreativeInstanceId)));
    ASSERT_TRUE(base::WriteFile(temp_dir_.GetPath()
                                    .AppendASCII(kRichMediaCreativeInstanceId)
                                    .AppendASCII("index.html"),
                                ""));
  }

  const base::FilePath& installed_dir() const { return temp_dir_.GetPath(); }

 private:
  base::ScopedTempDir temp_dir_;
};

TEST_F(NTPSponsoredRichMediaImagesDataTest, ParseSponsoredRichMediaCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredRichMediaCampaign);
  NTPSponsoredImagesData data(dict, installed_dir());
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  ASSERT_EQ(data.campaigns.size(), 1U);
  const auto& campaign = data.campaigns[0];
  EXPECT_EQ(campaign.campaign_id, "c27a3fae-ee9e-48a2-b3a7-f4675744e6ec");
  ASSERT_EQ(campaign.creatives.size(), 1U);
  const auto& creative = campaign.creatives[0];
  EXPECT_EQ(creative.wallpaper_type, WallpaperType::kRichMedia);
  EXPECT_EQ(creative.creative_instance_id, kRichMediaCreativeInstanceId);
  EXPECT_EQ(creative.url,
            GURL("chrome-untrusted://new-tab-takeover/"
                 "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"));
  EXPECT_EQ(creative.file_path, installed_dir()
                                    .AppendASCII(kRichMediaCreativeInstanceId)
                                    .AppendASCII("index.html"));
  EXPECT_EQ(creative.focal_point, gfx::Point(0, 0));

  EXPECT_EQ(creative.logo.company_name, "Another Rich Media NTT Creative");
  EXPECT_EQ(creative.logo.alt_text, "Some more rich content");
  EXPECT_EQ(creative.logo.destination_url, "https://basicattentiontoken.org");
  EXPECT_THAT(creative.logo.image_file.empty(), testing::IsTrue());
  EXPECT_THAT(creative.logo.image_url, testing::IsEmpty());
}

TEST_F(NTPSponsoredRichMediaImagesDataTest,
       GetCreativeByInstanceIdFromSponsoredRichMediaCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredRichMediaCampaign);
  NTPSponsoredImagesData data(dict, installed_dir());
  EXPECT_THAT(data.IsValid(), testing::IsTrue());

  EXPECT_EQ(data.GetCreativeByInstanceId(kRichMediaCreativeInstanceId),
            &data.campaigns[0].creatives[0]);
  EXPECT_EQ(
      data.GetCreativeByInstanceId("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"),
      nullptr);
}

}  // namespace ntp_background_images
