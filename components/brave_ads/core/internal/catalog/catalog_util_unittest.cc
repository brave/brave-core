/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogUtilTest : public UnitTestBase {};

TEST_F(BraveAdsCatalogUtilTest, ResetCatalog) {
  // Arrange
  SetCatalogId(kCatalogId);
  SetCatalogVersion(1);
  SetCatalogPing(base::Hours(2));
  SetCatalogLastUpdated(Now());

  // Act
  ResetCatalog();

  // Assert
  EXPECT_TRUE(!ads_client_mock_.HasPrefPath(prefs::kCatalogId) &&
              !ads_client_mock_.HasPrefPath(prefs::kCatalogVersion) &&
              !ads_client_mock_.HasPrefPath(prefs::kCatalogPing) &&
              !ads_client_mock_.HasPrefPath(prefs::kCatalogLastUpdated));
}

TEST_F(BraveAdsCatalogUtilTest, CatalogExists) {
  // Arrange
  SetCatalogVersion(1);

  // Act

  // Assert
  EXPECT_TRUE(DoesCatalogExist());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogDoesNotExist) {
  // Arrange
  SetCatalogVersion(0);

  // Act

  // Assert
  EXPECT_FALSE(DoesCatalogExist());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasChanged) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act

  // Assert
  EXPECT_TRUE(
      HasCatalogChanged(/*catalog_id*/ "150a9518-4db8-4fba-b104-0c420a1d9c0c"));
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasNotChanged) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act

  // Assert
  EXPECT_FALSE(HasCatalogChanged(kCatalogId));
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasExpired) {
  // Arrange
  SetCatalogLastUpdated(Now());

  // Act
  AdvanceClockBy(base::Days(1));

  // Assert
  EXPECT_TRUE(HasCatalogExpired());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasNotExpired) {
  // Arrange
  SetCatalogLastUpdated(Now());

  // Act
  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Assert
  EXPECT_FALSE(HasCatalogExpired());
}

}  // namespace brave_ads
