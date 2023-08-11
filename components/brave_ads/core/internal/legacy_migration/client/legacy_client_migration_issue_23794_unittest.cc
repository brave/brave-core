/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::client {

namespace {
constexpr char kClientIssue23794Filename[] = "client_issue_23794.json";
}  // namespace

class BraveAdsLegacyClientMigrationIssue23794Test : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    SetDefaultBooleanPref(prefs::kHasMigratedClientState, false);
  }
};

TEST_F(BraveAdsLegacyClientMigrationIssue23794Test, Migrate) {
  // Arrange
  CopyFileFromTestPathToTempPath(kClientIssue23794Filename,
                                 kClientStateFilename);

  // Act
  Migrate(base::BindOnce([](const bool success) {
    ASSERT_TRUE(success);

    // Assert
    EXPECT_TRUE(HasMigrated());
  }));

  // Assert
  EXPECT_TRUE(HasMigrated());
}

}  // namespace brave_ads::client
