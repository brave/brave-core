/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::client {

namespace {

constexpr char kClientIssue23794Filename[] = "client_issue_23794.json";
constexpr uint64_t kClientIssue23794JsonHash = 1891112954;
constexpr uint64_t kMigratedClientIssue23794JsonHash = 3078894446;

}  // namespace

class BraveAdsLegacyClientMigrationIssue23794Test : public UnitTestBase {};

TEST_F(BraveAdsLegacyClientMigrationIssue23794Test, Migrate) {
  // Arrange
  SetDefaultBooleanPref(prefs::kHasMigratedClientState, false);

  CopyFileFromTestPathToTempPath(kClientIssue23794Filename,
                                 kClientStateFilename);

  SetHash(kClientIssue23794JsonHash);

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedClientIssue23794JsonHash, GetHash());
}

}  // namespace brave_ads::client
