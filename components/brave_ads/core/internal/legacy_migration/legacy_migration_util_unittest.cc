/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"

#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsLegacyMigrationUtilTest : public test::TestBase {};

TEST_F(BraveAdsLegacyMigrationUtilTest, HasMigrated) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kHasMigratedState, true);

  // Act & Assert
  EXPECT_TRUE(HasMigratedState());
}

TEST_F(BraveAdsLegacyMigrationUtilTest, kHasMigratedState) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kHasMigratedState, false);

  // Act & Assert
  EXPECT_FALSE(HasMigratedState());
}

}  // namespace brave_ads
