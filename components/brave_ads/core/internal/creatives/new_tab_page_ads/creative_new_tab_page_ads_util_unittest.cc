/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"

#include <optional>
#include <utility>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/segments/segment_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

CreativeNewTabPageAdList BuildCreativeNewTabPageAds() {
  CreativeNewTabPageAdList creative_ads;

  {
    CreativeNewTabPageAdInfo creative_ad;
    creative_ad.campaign_id = "65933e82-6b21-440b-9956-c0f675ca7435";
    creative_ad.advertiser_id = "496b045a-195e-441f-b439-07bac083450f";
    creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;
    creative_ad.start_at = test::TimeFromString("2025-01-01T00:00:00Z");
    creative_ad.end_at = test::TimeFromString("2025-12-31T23:59:59Z");
    creative_ad.daily_cap = 20;
    creative_ad.priority = 10;
    creative_ad.pass_through_rate = 1;
    creative_ad.geo_targets = {"US"};
    creative_ad.dayparts = {{{"012", 0, 1439}, {"3456", 0, 1439}}};
    creative_ad.creative_set_id = "34ab06be-c9ed-4104-9ce0-9e639f4ad272";
    creative_ad.creative_instance_id = "aa0b561e-9eed-4aaa-8999-5627bc6b14fd";
    creative_ad.wallpaper_type = CreativeNewTabPageAdWallpaperType::kRichMedia;
    creative_ad.company_name = "Rich Media NTT Creative 1";
    creative_ad.alt = "Some rich content 1";
    creative_ad.target_url = GURL("https://brave.com/1");
    creative_ad.condition_matchers = {
        {/*pref_path*/ "uninstall_metrics.installation_date2",
         /*condition*/ "[T<]:7"},
        {/*pref_path*/ "[virtual]:operating_system|locale|language",
         /*condition*/ "en"}};
    creative_ad.segment = kUntargetedSegment;
    creative_ad.split_test_group = "Group A";
    creative_ad.per_day = 15;
    creative_ad.per_week = 120;
    creative_ad.per_month = 580;
    creative_ad.total_max = 980;
    creative_ad.value = 0.05;
    creative_ads.push_back(creative_ad);
  }

  {
    CreativeNewTabPageAdInfo creative_ad;
    creative_ad.campaign_id = "65933e82-6b21-440b-9956-c0f675ca7435";
    creative_ad.advertiser_id = "496b045a-195e-441f-b439-07bac083450f";
    creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;
    creative_ad.start_at = test::TimeFromString("2025-01-01T00:00:00Z");
    creative_ad.end_at = test::TimeFromString("2025-12-31T23:59:59Z");
    creative_ad.daily_cap = 20;
    creative_ad.priority = 10;
    creative_ad.pass_through_rate = 1;
    creative_ad.geo_targets = {"US"};
    creative_ad.dayparts = {{{"012", 0, 1439}, {"3456", 0, 1439}}};
    creative_ad.creative_set_id = "34ab06be-c9ed-4104-9ce0-9e639f4ad272";
    creative_ad.creative_instance_id = "143148ee-bd08-41f2-a8e0-fd8516e02975";
    creative_ad.wallpaper_type = CreativeNewTabPageAdWallpaperType::kImage;
    creative_ad.company_name = "Image NTT Creative 1";
    creative_ad.alt = "Some content 1";
    creative_ad.target_url = GURL("https://basicattentiontoken.org/1");
    creative_ad.condition_matchers = {
        {/*pref_path*/ "[virtual]:browser|version",
         /*condition*/ R"RE2(^\d+\.\d+\.(?:[0-6]?\d|7[0-7])\.\d+$)RE2"}};
    creative_ad.segment = kUntargetedSegment;
    creative_ad.split_test_group = "Group A";
    creative_ad.per_day = 15;
    creative_ad.per_week = 120;
    creative_ad.per_month = 580;
    creative_ad.total_max = 980;
    creative_ad.value = 0.05;
    creative_ads.push_back(creative_ad);
  }

  {
    CreativeNewTabPageAdInfo creative_ad;
    creative_ad.creative_instance_id = "5c2cf7fa-fc3f-4b1e-939a-7ac9fd2a128a";
    creative_ad.creative_set_id = "0c2f239c-1230-43f9-8759-9b17532c2749";
    creative_ad.campaign_id = "5a5e9915-128e-41af-9d56-b7c0db1ba6fa";
    creative_ad.advertiser_id = "496b045a-195e-441f-b439-07bac083450f";
    creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;
    creative_ad.start_at = test::TimeFromString("2025-03-01T00:00:00Z");
    creative_ad.end_at = test::TimeFromString("2025-09-31T23:59:59Z");
    creative_ad.daily_cap = 10;
    creative_ad.priority = 20;
    creative_ad.pass_through_rate = 0.5;
    creative_ad.per_day = 20;
    creative_ad.per_week = 140;
    creative_ad.per_month = 560;
    creative_ad.total_max = 1000;
    creative_ad.value = 0.1;
    creative_ad.segment = kUntargetedSegment;
    creative_ad.split_test_group = "Group B";
    creative_ad.condition_matchers = {
        {/*pref_path*/ "uninstall_metrics.installation_date2",
         /*condition*/ "[T<]:3"},
        {/*pref_path*/ "[virtual]:operating_system|locale|language",
         /*condition*/ "fr"}};
    creative_ad.dayparts = {{{"012", 0, 719}, {"3456", 720, 1439}}};
    creative_ad.geo_targets = {"KY"};
    creative_ad.wallpaper_type = CreativeNewTabPageAdWallpaperType::kRichMedia;
    creative_ad.company_name = "Rich Media NTT Creative 2";
    creative_ad.alt = "Some rich content 2";
    creative_ad.target_url = GURL("https://brave.com/2");
    creative_ads.push_back(creative_ad);
  }

  {
    CreativeNewTabPageAdInfo creative_ad;
    creative_ad.campaign_id = "5a5e9915-128e-41af-9d56-b7c0db1ba6fa";
    creative_ad.advertiser_id = "496b045a-195e-441f-b439-07bac083450f";
    creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;
    creative_ad.start_at = test::TimeFromString("2025-03-01T00:00:00Z");
    creative_ad.end_at = test::TimeFromString("2025-09-31T23:59:59Z");
    creative_ad.daily_cap = 10;
    creative_ad.priority = 20;
    creative_ad.pass_through_rate = 0.5;
    creative_ad.geo_targets = {"KY"};
    creative_ad.dayparts = {{{"012", 0, 719}, {"3456", 720, 1439}}};
    creative_ad.creative_set_id = "0c2f239c-1230-43f9-8759-9b17532c2749";
    creative_ad.creative_instance_id = "ec585946-7755-4ba1-8666-a65be845e1fd";
    creative_ad.wallpaper_type = CreativeNewTabPageAdWallpaperType::kImage;
    creative_ad.company_name = "Image NTT Creative 2";
    creative_ad.alt = "Some content 2";
    creative_ad.target_url = GURL("https://basicattentiontoken.org/2");
    creative_ad.condition_matchers = {
        {/*pref_path*/ "[virtual]:browser|version",
         /*condition*/ R"RE2(^\d+\.\d+\.(?:7[8-9]|[89]\d|\d{3,})\.\d+$)RE2"}};
    creative_ad.segment = kUntargetedSegment;
    creative_ad.split_test_group = "Group B";
    creative_ad.per_day = 20;
    creative_ad.per_week = 140;
    creative_ad.per_month = 560;
    creative_ad.total_max = 1000;
    creative_ad.value = 0.1;
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace

class BraveAdsCreativeNewTabPageAdsUtilTest : public test::TestBase {};

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest, ParseAndSaveAds) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("4 July 2025"));

  base::Value::Dict dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 2,
        "campaigns": [
          {
            "version": 1,
            "campaignId": "65933e82-6b21-440b-9956-c0f675ca7435",
            "advertiserId": "496b045a-195e-441f-b439-07bac083450f",
            "startAt": "2025-01-01T00:00:00Z",
            "endAt": "2025-12-31T23:59:59Z",
            "dailyCap": 20,
            "priority": 10,
            "ptr": 1,
            "metrics": "confirmation",
            "geoTargets": [
              "US"
            ],
            "dayParts": [
              {
                "daysOfWeek": "012",
                "startMinute": 0,
                "endMinute": 1439
              },
              {
                "daysOfWeek": "3456",
                "startMinute": 0,
                "endMinute": 1439
              }
            ],
            "creativeSets": [
              {
                "creativeSetId": "34ab06be-c9ed-4104-9ce0-9e639f4ad272",
                "creatives": [
                  {
                    "creativeInstanceId": "aa0b561e-9eed-4aaa-8999-5627bc6b14fd",
                    "companyName": "Rich Media NTT Creative 1",
                    "alt": "Some rich content 1",
                    "targetUrl": "https://brave.com/1",
                    "conditionMatchers": [
                      {
                        "condition": "[T<]:7",
                        "prefPath": "uninstall_metrics.installation_date2"
                      },
                      {
                        "condition": "en",
                        "prefPath": "[virtual]:operating_system|locale|language"
                      }
                    ],
                    "wallpaper": {
                      "type": "richMedia",
                      "relativeUrl": "689fcd95-c77c-4574-9615-ab8cf1995735/index.html"
                    }
                  },
                  {
                    "creativeInstanceId": "143148ee-bd08-41f2-a8e0-fd8516e02975",
                    "companyName": "Image NTT Creative 1",
                    "alt": "Some content 1",
                    "targetUrl": "https://basicattentiontoken.org/1",
                    "conditionMatchers": [
                      {
                        "condition": "^\\d+\\.\\d+\\.(?:[0-6]?\\d|7[0-7])\\.\\d+$",
                        "prefPath": "[virtual]:browser|version"
                      }
                    ],
                    "wallpaper": {
                      "type": "image",
                      "relativeUrl": "f4c33efb-1605-4b2a-8980-4d41d25b36ff/background.jpg",
                      "focalPoint": {
                        "x": 25,
                        "y": 50
                      },
                      "button": {
                        "image": {
                          "relativeUrl": "eee468e8-d56f-4d59-9b51-03e1b1069fb3/button.png"
                        }
                      }
                    }
                  }
                ],
                "conversions": [
                  {
                    "observationWindow": 7,
                    "urlPattern": "https://www.brave.com/1/*"
                  }
                ],
                "segments": [
                  "untargeted"
                ],
                "splitTestGroup": "Group A",
                "perDay": 15,
                "perWeek": 120,
                "perMonth": 580,
                "totalMax": 980,
                "value": "0.05"
              }
            ]
          },
          {
            "version": 1,
            "campaignId": "5a5e9915-128e-41af-9d56-b7c0db1ba6fa",
            "advertiserId": "496b045a-195e-441f-b439-07bac083450f",
            "startAt": "2025-03-01T00:00:00Z",
            "endAt": "2025-09-31T23:59:59Z",
            "dailyCap": 10,
            "priority": 20,
            "ptr": 0.5,
            "metrics": "disabled",
            "geoTargets": [
              "KY"
            ],
            "dayParts": [
              {
                "daysOfWeek": "012",
                "startMinute": 0,
                "endMinute": 719
              },
              {
                "daysOfWeek": "3456",
                "startMinute": 720,
                "endMinute": 1439
              }
            ],
            "creativeSets": [
              {
                "creativeSetId": "0c2f239c-1230-43f9-8759-9b17532c2749",
                "creatives": [
                  {
                    "creativeInstanceId": "5c2cf7fa-fc3f-4b1e-939a-7ac9fd2a128a",
                    "companyName": "Rich Media NTT Creative 2",
                    "alt": "Some rich content 2",
                    "targetUrl": "https://brave.com/2",
                    "conditionMatchers": [
                      {
                        "condition": "[T<]:3",
                        "prefPath": "uninstall_metrics.installation_date2"
                      },
                      {
                        "condition": "fr",
                        "prefPath": "[virtual]:operating_system|locale|language"
                      }
                    ],
                    "wallpaper": {
                      "type": "richMedia",
                      "relativeUrl": "748eafb6-c96f-4a54-87dc-a39c2718c45e/index.html"
                    }
                  },
                  {
                    "creativeInstanceId": "ec585946-7755-4ba1-8666-a65be845e1fd",
                    "companyName": "Image NTT Creative 2",
                    "alt": "Some content 2",
                    "targetUrl": "https://basicattentiontoken.org/2",
                    "conditionMatchers": [
                      {
                        "condition": "^\\d+\\.\\d+\\.(?:7[8-9]|[89]\\d|\\d{3,})\\.\\d+$",
                        "prefPath": "[virtual]:browser|version"
                      }
                    ],
                    "wallpaper": {
                      "type": "image",
                      "relativeUrl": "b12b0568-6894-42c8-abf8-4997dabb6c95/background.jpg",
                      "focalPoint": {
                        "x": 25,
                        "y": 50
                      },
                      "button": {
                        "image": {
                          "relativeUrl": "b12b0568-6894-42c8-abf8-4997dabb6c95/button.png"
                        }
                      }
                    }
                  }
                ],
                "conversions": [
                  {
                    "observationWindow": 30,
                    "urlPattern": "https://www.brave.com/2/*"
                  }
                ],
                "segments": [
                  "untargeted"
                ],
                "splitTestGroup": "Group B",
                "perDay": 20,
                "perWeek": 140,
                "perMonth": 560,
                "totalMax": 1000,
                "value": "0.1"
              }
            ]
          }
        ]
      })JSON");

  // Act
  {
    base::MockCallback<ResultCallback> callback;
    base::RunLoop run_loop;
    EXPECT_CALL(callback, Run(/*success=*/true))
        .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
    ParseAndSaveNewTabPageAds(std::move(dict), callback.Get());
    run_loop.Run();
  }

  // Assert
  base::MockCallback<database::table::GetCreativeNewTabPageAdsCallback>
      callback;
  base::RunLoop run_loop;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true, SegmentList{kUntargetedSegment},
          ::testing::UnorderedElementsAreArray(BuildCreativeNewTabPageAds())))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForActiveCampaigns(callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest,
       ParseAndSaveAdsWithEmptyCampaigns) {
  // Arrange
  base::Value::Dict dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 2,
        "campaigns": []
      })JSON");

  // Act & Assert
  base::MockCallback<ResultCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  ParseAndSaveNewTabPageAds(std::move(dict), callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest,
       DoNotParseAndSaveAdsWithInvalidData) {
  // Act & Assert
  base::MockCallback<ResultCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/false))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  ParseAndSaveNewTabPageAds(base::Value::Dict(), callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest,
       DoNotParseAndSaveAdsWithUnsupportedSchemaVersion) {
  // Arrange
  base::Value::Dict dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 0,
        "campaigns": []
      })JSON");

  // Act & Assert
  base::MockCallback<ResultCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/false))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  ParseAndSaveNewTabPageAds(std::move(dict), callback.Get());
  run_loop.Run();
}

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest,
       DoNotParseAndSaveAdsWithMissingCampaigns) {
  // Arrange
  base::Value::Dict dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 2,
      })JSON");

  // Act & Assert
  base::MockCallback<ResultCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/false))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  ParseAndSaveNewTabPageAds(std::move(dict), callback.Get());
  run_loop.Run();
}

TEST(BraveAdsNewTabPageAdMetricTypeUtilTest, ToMojomNewTabPageAdMetricType) {
  // Act & Assert
  EXPECT_EQ(ToMojomNewTabPageAdMetricType("disabled"),
            mojom::NewTabPageAdMetricType::kDisabled);

  EXPECT_EQ(ToMojomNewTabPageAdMetricType("confirmation"),
            mojom::NewTabPageAdMetricType::kConfirmation);

  EXPECT_EQ(ToMojomNewTabPageAdMetricType("foobar"), std::nullopt);
}

TEST(BraveAdsNewTabPageAdMetricTypeUtilTest, ToString) {
  // Act & Assert
  EXPECT_EQ(ToString(mojom::NewTabPageAdMetricType::kUndefined), "");

  EXPECT_EQ(ToString(mojom::NewTabPageAdMetricType::kDisabled), "disabled");

  EXPECT_EQ(ToString(mojom::NewTabPageAdMetricType::kConfirmation),
            "confirmation");
}

}  // namespace brave_ads
