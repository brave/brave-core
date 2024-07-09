/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogPermissionRuleIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override { test::TestBase::SetUp(/*is_integration_test=*/true); }

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK, /*response_body=*/"/catalog.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BraveAdsCatalogPermissionRuleIntegrationTest, ShouldAllow) {
  // Act & Assert
  EXPECT_TRUE(HasCatalogPermission());
}

TEST_F(BraveAdsCatalogPermissionRuleIntegrationTest,
       ShouldAllowIfCatalogWasLastUpdated23HoursAnd59MinutesAgo) {
  // Arrange
  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_TRUE(HasCatalogPermission());
}

TEST_F(BraveAdsCatalogPermissionRuleIntegrationTest,
       ShouldNotAllowIfCatalogWasLastUpdated1DayAgo) {
  // Arrange
  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_FALSE(HasCatalogPermission());
}

TEST_F(BraveAdsCatalogPermissionRuleIntegrationTest,
       ShouldNotAllowIfCatalogDoesNotExist) {
  // Arrange
  SetCatalogVersion(0);

  // Act & Assert
  EXPECT_FALSE(HasCatalogPermission());
}

}  // namespace brave_ads
