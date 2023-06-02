/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_unittest_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::confirmations {

namespace {

constexpr uint64_t kConfirmationJsonHash = 1891112954;
constexpr uint64_t kMigratedConfirmationJsonHash = 3830595452;

constexpr char kInvalidJsonFilename[] = "invalid.json";

}  // namespace

class BraveAdsLegacyConfirmationMigrationTest : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    SetDefaultBooleanPref(prefs::kHasMigratedConfirmationState, false);
  }
};

TEST_F(BraveAdsLegacyConfirmationMigrationTest, Migrate) {
  // Arrange
  CopyFileFromTestPathToTempPath(kConfirmationStateFilename);

  SetHash(kConfirmationJsonHash);

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedConfirmationJsonHash, GetHash());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest, InvalidState) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonFilename,
                                 kConfirmationStateFilename);

  // Act
  Migrate(/*should_migrate*/ false);

  // Assert
  EXPECT_FALSE(HasMigrated());
}

TEST_F(BraveAdsLegacyConfirmationMigrationTest, AlreadyMigrated) {
  // Arrange
  CopyFileFromTestPathToTempPath(kConfirmationStateFilename);

  SetHash(kConfirmationJsonHash);

  Migrate(/*should_migrate*/ true);
  ASSERT_EQ(kMigratedConfirmationJsonHash, GetHash());

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedConfirmationJsonHash, GetHash());
}

}  // namespace brave_ads::confirmations
