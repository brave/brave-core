/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::confirmations {

namespace {
constexpr char kInvalidJsonFilename[] = "invalid.json";
}  // namespace

class BraveAdsLegacyConfirmationMigrationTest : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    SetBooleanPref(prefs::kHasMigratedConfirmationState, false);
  }
};

TEST_F(BraveAdsLegacyConfirmationMigrationTest, Migrate) {
  // Arrange
  CopyFileFromTestPathToTempPath(kConfirmationStateFilename);

  // Act
  Migrate(base::BindOnce([](const bool success) {
    ASSERT_TRUE(success);

    // Assert
    EXPECT_TRUE(HasMigrated());
  }));
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest, InvalidState) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonFilename,
                                 kConfirmationStateFilename);

  // Act
  Migrate(base::BindOnce([](const bool success) {
    // Assert
    EXPECT_FALSE(success);
  }));
}

}  // namespace brave_ads::confirmations
