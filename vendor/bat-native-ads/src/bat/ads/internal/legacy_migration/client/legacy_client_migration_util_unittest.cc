/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/client/legacy_client_migration_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::client {

class BatAdsLegacyClientMigrationUtilTest : public UnitTestBase {};

TEST_F(BatAdsLegacyClientMigrationUtilTest, HasMigrated) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kHasMigratedClientState,
                                                 true);

  // Act

  // Assert
  EXPECT_TRUE(HasMigrated());
}

TEST_F(BatAdsLegacyClientMigrationUtilTest, HasNotMigrated) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kHasMigratedClientState,
                                                 false);

  // Act

  // Assert
  EXPECT_FALSE(HasMigrated());
}

}  // namespace ads::client
