/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeNotificationAdsDatabaseTableIntegrationTest
    : public test::TestBase {
 public:
  BraveAdsCreativeNotificationAdsDatabaseTableIntegrationTest()
      : test::TestBase(/*is_integration_test=*/true) {}

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK, /*response_body=*/"/catalog.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableIntegrationTest,
       GetForSegments) {
  // Arrange
  const database::table::CreativeNotificationAds database_table;

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([&] {
    base::test::TestFuture<bool, SegmentList, CreativeNotificationAdList>
        test_future;
    database_table.GetForSegments(
        /*segments=*/{"technology & computing"},
        test_future.GetCallback<bool, const SegmentList&,
                                CreativeNotificationAdList>());
    const auto [success, segments, creative_ads] = test_future.Take();
    if (!success || creative_ads.empty()) {
      return false;
    }
    EXPECT_THAT(creative_ads, ::testing::SizeIs(2));
    return true;
  }));
}

}  // namespace brave_ads
