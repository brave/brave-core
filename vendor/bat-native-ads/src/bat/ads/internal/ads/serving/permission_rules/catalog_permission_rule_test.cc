/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/catalog_permission_rule.h"

#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogPermissionRuleIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsCatalogPermissionRuleIntegrationTest, AllowAd) {
  // Arrange

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleIntegrationTest,
       AllowAdIfCatalogWasLastUpdated23HoursAnd59MinutesAgo) {
  // Arrange
  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleIntegrationTest,
       DoNotAllowAdIfCatalogWasLastUpdated1DayAgo) {
  // Arrange
  AdvanceClockBy(base::Days(1));

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleIntegrationTest,
       DoNotAllowAdIfCatalogDoesNotExist) {
  // Arrange
  SetCatalogVersion(0);

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
