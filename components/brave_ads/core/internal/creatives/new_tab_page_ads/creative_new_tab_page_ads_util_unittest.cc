/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeNewTabPageAdsUtilTest : public test::TestBase {};

TEST_F(BraveAdsCreativeNewTabPageAdsUtilTest, ParseAndSaveAds) {
  // Arrange
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
                    "companyName": "Rich Media NTT Creative",
                    "alt": "Some rich content",
                    "targetUrl": "https://brave.com",
                    "conditionMatchers": [
                      {
                        "condition": "[T<]:7",
                        "prefPath": "uninstall_metrics.installation_date2"
                      }
                    ],
                    "wallpaper": {
                      "type": "richMedia",
                      "relativeUrl": "aa0b561e-9eed-4aaa-8999-5627bc6b14fd/index.html"
                    }
                  },
                  {
                    "creativeInstanceId": "143148ee-bd08-41f2-a8e0-fd8516e02975",
                    "companyName": "Image NTT Creative",
                    "alt": "Some content",
                    "targetUrl": "https://basicattentiontoken.org",
                    "wallpaper": {
                      "type": "image",
                      "relativeUrl": "143148ee-bd08-41f2-a8e0-fd8516e02975/background.jpg",
                      "focalPoint": {
                        "x": 25,
                        "y": 50
                      },
                      "button": {
                        "image": {
                          "relativeUrl": "143148ee-bd08-41f2-a8e0-fd8516e02975/button.png"
                        }
                      }
                    }
                  }
                ],
                "conversions": [
                  {
                    "observationWindow": 30,
                    "urlPattern": "https://www.brave.com/*"
                  }
                ],
                "segments": [
                  "technology & computing"
                ],
                "splitTestGroup": "Group A",
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

  // Act & Assert
  base::MockCallback<ResultCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, Run(/*success=*/true))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));
  ParseAndSaveNewTabPageAds(std::move(dict), callback.Get());
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

}  // namespace brave_ads
