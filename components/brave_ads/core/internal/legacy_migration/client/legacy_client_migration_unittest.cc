/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::client {

namespace {
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

  // Act
  Migrate(base::BindOnce([](const bool success) {
    ASSERT_TRUE(success);

    // Assert
    EXPECT_TRUE(HasMigrated());
  }));
}

TEST_F(BraveAdsLegacyClientMigrationTest, InvalidState) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonFilename, kClientStateFilename);

  // Act
  Migrate(base::BindOnce([](const bool success) {
    // Assert
    EXPECT_FALSE(success);
  }));
}

}  // namespace brave_ads::client
