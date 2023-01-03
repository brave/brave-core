/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogUtilTest : public UnitTestBase {};

TEST_F(BatAdsCatalogUtilTest, ResetCatalog) {
  // Arrange
  SetCatalogId("150a9518-4db8-4fba-b104-0c420a1d9c0c");
  SetCatalogVersion(1);
  SetCatalogPing(base::Hours(2));
  SetCatalogLastUpdated(Now());

  // Act
  ResetCatalog();

  // Assert
  EXPECT_TRUE(
      !AdsClientHelper::GetInstance()->HasPrefPath(prefs::kCatalogId) &&
      !AdsClientHelper::GetInstance()->HasPrefPath(prefs::kCatalogVersion) &&
      !AdsClientHelper::GetInstance()->HasPrefPath(prefs::kCatalogPing) &&
      !AdsClientHelper::GetInstance()->HasPrefPath(prefs::kCatalogLastUpdated));
}

TEST_F(BatAdsCatalogUtilTest, CatalogExists) {
  // Arrange
  SetCatalogVersion(1);

  // Act
  const bool does_exist = DoesCatalogExist();

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsCatalogUtilTest, CatalogDoesNotExist) {
  // Arrange
  SetCatalogVersion(0);

  // Act
  const bool does_exist = DoesCatalogExist();

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasChanged) {
  // Arrange
  SetCatalogId("29e5c8bc0ba319069980bb390d8e8f9b58c05a20");

  // Act

  // Assert
  const bool has_changed =
      HasCatalogChanged("150a9518-4db8-4fba-b104-0c420a1d9c0c");
  EXPECT_TRUE(has_changed);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasNotChanged) {
  // Arrange
  SetCatalogId("29e5c8bc0ba319069980bb390d8e8f9b58c05a20");

  // Act

  // Assert
  const bool has_changed =
      HasCatalogChanged("29e5c8bc0ba319069980bb390d8e8f9b58c05a20");
  EXPECT_FALSE(has_changed);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasExpired) {
  // Arrange
  SetCatalogLastUpdated(Now());

  // Act
  AdvanceClockBy(base::Days(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_TRUE(has_expired);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasNotExpired) {
  // Arrange
  SetCatalogLastUpdated(Now());

  // Act
  AdvanceClockBy(base::Days(1) - base::Seconds(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_FALSE(has_expired);
}

}  // namespace ads
