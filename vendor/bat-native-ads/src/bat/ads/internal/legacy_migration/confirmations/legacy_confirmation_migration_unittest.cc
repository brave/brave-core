/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration_unittest_util.h"
#include "bat/ads/internal/legacy_migration/confirmations/legacy_confirmation_migration_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::confirmations {

namespace {

constexpr uint64_t kConfirmationJsonHash = 1891112954;
constexpr uint64_t kMigratedConfirmationJsonHash = 3830595452;

constexpr char kInvalidJsonFilename[] = "invalid.json";

}  // namespace

class BatAdsLegacyConfirmationMigrationTest : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kHasMigratedConfirmationState, false);
  }
};

TEST_F(BatAdsLegacyConfirmationMigrationTest, Migrate) {
  // Arrange
  CopyFileFromTestPathToTempPath(kConfirmationStateFilename);

  SetHash(kConfirmationJsonHash);

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedConfirmationJsonHash, GetHash());
}

TEST_F(BatAdsLegacyConfirmationMigrationTest, InvalidState) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonFilename,
                                 kConfirmationStateFilename);

  // Act
  Migrate(/*should_migrate*/ false);

  // Assert
  EXPECT_FALSE(HasMigrated());
}

TEST_F(BatAdsLegacyConfirmationMigrationTest, AlreadyMigrated) {
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

}  // namespace ads::confirmations
