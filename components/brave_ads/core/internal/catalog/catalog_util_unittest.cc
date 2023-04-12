/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

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
  EXPECT_TRUE(!ads_client_mock_->HasPrefPath(prefs::kCatalogId) &&
              !ads_client_mock_->HasPrefPath(prefs::kCatalogVersion) &&
              !ads_client_mock_->HasPrefPath(prefs::kCatalogPing) &&
              !ads_client_mock_->HasPrefPath(prefs::kCatalogLastUpdated));
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
  SetCatalogId(kCatalogId);

  // Act

  // Assert
  const bool has_changed =
      HasCatalogChanged("150a9518-4db8-4fba-b104-0c420a1d9c0c");
  EXPECT_TRUE(has_changed);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasNotChanged) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act

  // Assert
  const bool has_changed = HasCatalogChanged(kCatalogId);
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
  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_FALSE(has_expired);
}

}  // namespace brave_ads
