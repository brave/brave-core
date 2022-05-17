/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/catalog/catalog_util.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCatalogUtilTest : public UnitTestBase {
 protected:
  BatAdsCatalogUtilTest() = default;

  ~BatAdsCatalogUtilTest() override = default;
};

TEST_F(BatAdsCatalogUtilTest, ResetCatalog) {
  // Arrange
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId,
                                        "150a9518-4db8-4fba-b104-0c420a1d9c0c");
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 1);
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogPing, 1000);
  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        NowAsTimestamp());

  // Act
  ResetCatalog();

  // Assert
  EXPECT_TRUE(!AdsClientHelper::Get()->HasPrefPath(prefs::kCatalogId) &&
              !AdsClientHelper::Get()->HasPrefPath(prefs::kCatalogVersion) &&
              !AdsClientHelper::Get()->HasPrefPath(prefs::kCatalogPing) &&
              !AdsClientHelper::Get()->HasPrefPath(prefs::kCatalogLastUpdated));
}

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
  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        NowAsTimestamp());

  // Act
  AdvanceClock(base::Days(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_TRUE(has_expired);
}

TEST_F(BatAdsCatalogUtilTest, CatalogHasNotExpired) {
  // Arrange
  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        NowAsTimestamp());

  // Act
  AdvanceClock(base::Days(1) - base::Seconds(1));

  // Assert
  const bool has_expired = HasCatalogExpired();
  EXPECT_FALSE(has_expired);
}

}  // namespace ads
