/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsCatalogFrequencyCapTest() = default;

  ~BatAdsCatalogFrequencyCapTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsCatalogFrequencyCapTest, AllowAd) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act
  CatalogFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogFrequencyCapTest,
       AllowAdIfCatalogWasLastUpdated23HoursAnd59MinutesAgo) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(base::TimeDelta::FromHours(23) +
               base::TimeDelta::FromMinutes(59));

  // Act
  CatalogFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCatalogFrequencyCapTest,
       DoNotAllowAdIfCatalogWasLastUpdated1DayAgo) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  AdvanceClock(base::TimeDelta::FromDays(1));

  // Act
  CatalogFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsCatalogFrequencyCapTest, DoNotAllowAdIfCatalogDoesNotExist) {
  // Arrange
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 0);

  // Act
  CatalogFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
