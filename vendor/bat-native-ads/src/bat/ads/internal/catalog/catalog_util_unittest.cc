/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_util.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogUtilTest : public UnitTestBase {
 protected:
  BatAdsCatalogUtilTest() = default;

  ~BatAdsCatalogUtilTest() override = default;
};

TEST_F(BatAdsCatalogUtilTest, CatalogExists) {
  // Arrange
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 1);

  // Act
  const bool does_exist = DoesCatalogExist();

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST_F(BatAdsCatalogUtilTest, CatalogDoesNotExist) {
  // Arrange
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 0);

  // Act
  const bool does_exist = DoesCatalogExist();

  // Assert
  EXPECT_FALSE(does_exist);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasExpired) {
  // Arrange
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogLastUpdated,
                                       NowAsTimestamp());

  // Act
  AdvanceClock(base::TimeDelta::FromDays(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_TRUE(has_expired);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasNotExpired) {
  // Arrange
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogLastUpdated,
                                       NowAsTimestamp());

  // Act
  AdvanceClock(base::TimeDelta::FromDays(1) - base::TimeDelta::FromSeconds(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_FALSE(has_expired);
}

}  // namespace ads
