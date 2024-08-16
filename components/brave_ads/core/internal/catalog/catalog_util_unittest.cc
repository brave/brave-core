/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogUtilTest : public test::TestBase {};

TEST_F(BraveAdsCatalogUtilTest, ResetCatalog) {
  // Arrange
  SetCatalogId(test::kCatalogId);
  SetCatalogVersion(1);
  SetCatalogPing(base::Hours(1));
  SetCatalogLastUpdated(test::Now());

  // Act
  ResetCatalog();

  // Assert
  EXPECT_THAT(GetProfileStringPref(prefs::kCatalogId), ::testing::IsEmpty());
  EXPECT_EQ(0, GetProfileIntegerPref(prefs::kCatalogVersion));
  EXPECT_EQ(7'200'000, GetProfileInt64Pref(prefs::kCatalogPing));
  EXPECT_TRUE(GetProfileTimePref(prefs::kCatalogLastUpdated).is_null());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogExists) {
  // Arrange
  SetCatalogVersion(1);

  // Act & Assert
  EXPECT_TRUE(DoesCatalogExist());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogDoesNotExist) {
  // Arrange
  SetCatalogVersion(0);

  // Act & Assert
  EXPECT_FALSE(DoesCatalogExist());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasChanged) {
  // Arrange
  SetCatalogId(test::kCatalogId);

  // Act & Assert
  EXPECT_TRUE(HasCatalogChanged(
      /*catalog_id=*/"150a9518-4db8-4fba-b104-0c420a1d9c0c"));
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasNotChanged) {
  // Arrange
  SetCatalogId(test::kCatalogId);

  // Act & Assert
  EXPECT_FALSE(HasCatalogChanged(test::kCatalogId));
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasExpired) {
  // Arrange
  SetCatalogLastUpdated(test::Now());

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_TRUE(HasCatalogExpired());
}

TEST_F(BraveAdsCatalogUtilTest, CatalogHasNotExpired) {
  // Arrange
  SetCatalogLastUpdated(test::Now());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(HasCatalogExpired());
}

}  // namespace brave_ads
