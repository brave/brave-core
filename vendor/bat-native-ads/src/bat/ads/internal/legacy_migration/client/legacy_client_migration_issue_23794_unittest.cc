/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/client/legacy_client_migration.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/client/client_state_manager_constants.h"
#include "bat/ads/internal/legacy_migration/client/legacy_client_migration_unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace client {

namespace {

constexpr char kClientIssue23794Filename[] = "client_issue_23794.json";
constexpr uint64_t kClientIssue23794JsonHash = 1891112954;
constexpr uint64_t kMigratedClientIssue23794JsonHash = 1570672789;

}  // namespace

class BatAdsLegacyClientMigrationIssue23794Test : public UnitTestBase {
 protected:
  BatAdsLegacyClientMigrationIssue23794Test() = default;

  ~BatAdsLegacyClientMigrationIssue23794Test() override = default;
};

TEST_F(BatAdsLegacyClientMigrationIssue23794Test, Migrate) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kHasMigratedClientState,
                                                 false);

  CopyFileFromTestPathToTempPath(kClientIssue23794Filename,
                                 kClientStateFilename);

  SetHash(kClientIssue23794JsonHash);

  // Act
  Migrate(/*should_migrate*/ true);

  // Assert
  EXPECT_EQ(kMigratedClientIssue23794JsonHash, GetHash());
}

}  // namespace client
}  // namespace ads
