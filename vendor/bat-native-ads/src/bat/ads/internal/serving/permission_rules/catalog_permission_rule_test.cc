/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/catalog_permission_rule.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsCatalogPermissionRuleTest() = default;

  ~BatAdsCatalogPermissionRuleTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }
};

TEST_F(BatAdsCatalogPermissionRuleTest, AllowAd) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleTest,
       AllowAdIfCatalogWasLastUpdated23HoursAnd59MinutesAgo) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(base::Hours(23) + base::Minutes(59));

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleTest,
       DoNotAllowAdIfCatalogWasLastUpdated1DayAgo) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(base::Days(1));

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsCatalogPermissionRuleTest, DoNotAllowAdIfCatalogDoesNotExist) {
  // Arrange
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 0);

  // Act
  CatalogPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
