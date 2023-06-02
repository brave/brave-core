/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_unittest_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::client {

namespace {

constexpr uint64_t kClientJsonHash = 1891112954;
constexpr uint64_t kMigratedClientJsonHash = 1204433941;

constexpr char kInvalidJsonFilename[] = "invalid.json";

}  // namespace

class BraveAdsLegacyClientMigrationTest : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    SetDefaultBooleanPref(prefs::kHasMigratedClientState, false);
  }
};

TEST_F(BraveAdsLegacyClientMigrationTest, Migrate) {
  // Arrange
  CopyFileFromTestPathToTempPath(kClientStateFilename);

  SetHash(kClientJsonHash);

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedClientJsonHash, GetHash());
}

TEST_F(BraveAdsLegacyClientMigrationTest, InvalidState) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonFilename, kClientStateFilename);

  // Act
  Migrate(/*should_migrate*/ false);

  // Assert
  EXPECT_FALSE(HasMigrated());
}

TEST_F(BraveAdsLegacyClientMigrationTest, AlreadyMigrated) {
  // Arrange
  CopyFileFromTestPathToTempPath(kClientStateFilename);

  SetHash(kClientJsonHash);

  Migrate(/*should_migrate*/ true);
  ASSERT_EQ(kMigratedClientJsonHash, GetHash());

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedClientJsonHash, GetHash());
}

}  // namespace brave_ads::client
