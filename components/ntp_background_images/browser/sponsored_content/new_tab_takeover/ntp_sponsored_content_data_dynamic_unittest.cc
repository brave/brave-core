/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/dcheck_is_on.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/ntp_sponsored_content_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

constexpr char kDynamicCreativeInstanceId[] =
    "39d78863-327d-4b64-9952-cd0e5e330eb6";

constexpr char kTestSponsoredDynamicCampaign[] = R"(
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

class NTPSponsoredContentDataDynamicTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::CreateDirectory(
        temp_dir_.GetPath().AppendASCII(kDynamicCreativeInstanceId)));
    ASSERT_TRUE(base::WriteFile(temp_dir_.GetPath()
                                    .AppendASCII(kDynamicCreativeInstanceId)
                                    .AppendASCII("index.html"),
                                ""));
  }

  const base::FilePath& installed_dir() const { return temp_dir_.GetPath(); }

 private:
  base::ScopedTempDir temp_dir_;
};

TEST_F(NTPSponsoredContentDataDynamicTest, ParseSponsoredDynamicCampaign) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredDynamicCampaign);
  NTPSponsoredContentData sponsored_content_data(dict, installed_dir());
  EXPECT_TRUE(sponsored_content_data.IsValid());

  EXPECT_THAT(
      sponsored_content_data.campaigns,
      testing::ElementsAre(testing::FieldsAre(
          /*campaign_id=*/"c27a3fae-ee9e-48a2-b3a7-f4675744e6ec",
          /*creatives=*/
          testing::ElementsAre(testing::FieldsAre(
              WallpaperType::kDynamicNewTabTakeover,
              GURL("chrome-untrusted://new-tab-takeover/"
                   "39d78863-327d-4b64-9952-cd0e5e330eb6/index.html"),
              /*file_path=*/
              installed_dir()
                  .AppendASCII(kDynamicCreativeInstanceId)
                  .AppendASCII("index.html"),
              gfx::Point(), kDynamicCreativeInstanceId,
              brave_ads::mojom::NewTabPageAdMetricType::kConfirmation,
              /*logo=*/
              testing::FieldsAre(base::FilePath(),
                                 /*image_url=*/testing::IsEmpty(),
                                 /*alt_text=*/"Some more rich content",
                                 /*destination_url=*/
                                 "https://basicattentiontoken.org",
                                 /*company_name=*/
                                 "Another Rich Media NTT Creative"))))));
}

TEST_F(NTPSponsoredContentDataDynamicTest,
       GetCreativeByInstanceIdReturnsCreativeForKnownInstanceId) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredDynamicCampaign);
  NTPSponsoredContentData sponsored_content_data(dict, installed_dir());
  EXPECT_TRUE(sponsored_content_data.IsValid());

  const Creative* const creative =
      sponsored_content_data.GetCreativeByInstanceId(
          kDynamicCreativeInstanceId);
  ASSERT_TRUE(creative);
  EXPECT_EQ(creative->creative_instance_id, kDynamicCreativeInstanceId);
}

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPSponsoredContentDataDynamicTest,
       GetCreativeByInstanceIdReturnsNullForUnknownInstanceId) {
  base::DictValue dict =
      base::test::ParseJsonDict(kTestSponsoredDynamicCampaign);
  NTPSponsoredContentData sponsored_content_data(dict, installed_dir());
  EXPECT_TRUE(sponsored_content_data.IsValid());

  EXPECT_FALSE(sponsored_content_data.GetCreativeByInstanceId(
      "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace ntp_background_images
