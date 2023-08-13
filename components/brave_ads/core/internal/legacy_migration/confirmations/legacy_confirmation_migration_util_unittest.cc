/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::confirmations {

class BraveAdsLegacyConfirmationMigrationUtilTest : public UnitTestBase {};

TEST_F(BraveAdsLegacyConfirmationMigrationUtilTest, HasMigrated) {
  // Arrange
  SetBooleanPref(prefs::kHasMigratedConfirmationState, true);

  // Act

  // Assert
  EXPECT_TRUE(HasMigrated());
}

TEST_F(BraveAdsLegacyConfirmationMigrationUtilTest, HasNotMigrated) {
  // Arrange
  SetBooleanPref(prefs::kHasMigratedConfirmationState, false);

  // Act

  // Assert
  EXPECT_FALSE(HasMigrated());
}

}  // namespace brave_ads::confirmations
