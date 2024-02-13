/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

std::string TestParamToString(::testing::TestParamInfo<int> test_param) {
  return base::StringPrintf("%d_to_%d", test_param.param, database::kVersion);
}

}  // namespace

class BraveAdsDatabaseMigrationTest : public UnitTestBase,
                                      public ::testing::WithParamInterface<int>,
                                      public DatabaseManagerObserver {
 protected:
  void SetUpMocks() override {
    MaybeMockDatabase();

    DatabaseManager::GetInstance().AddObserver(this);
  }

  void TearDown() override {
    DatabaseManager::GetInstance().RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  static int GetSchemaVersion() { return GetParam(); }

  static bool ShouldCreateDatabase() { return GetSchemaVersion() == 0; }

  static bool IsMostRecentSchemaVersion() {
    return GetSchemaVersion() == database::kVersion;
  }

  static bool ShouldMigrateDatabase() {
    return !ShouldCreateDatabase() && !IsMostRecentSchemaVersion();
  }

  void MaybeMockDatabase() {
    if (ShouldCreateDatabase()) {
      return;
    }

    const std::string database_filename = base::StringPrintf(
        "database/database_schema_%d.sqlite", GetSchemaVersion());
    ASSERT_TRUE(
        CopyFileFromTestPathToTempPath(database_filename, kDatabaseFilename));
  }

  // DatabaseManagerObserver:
  void OnDidCreateDatabase() override { did_create_database_ = true; }

  void OnDidMigrateDatabase(const int /*from_version*/,
                            const int /*to_version*/) override {
    did_migrate_database_ = true;
  }

  void OnFailedToMigrateDatabase(const int /*from_version*/,
                                 const int /*to_version*/) override {
    failed_to_migrate_database_ = true;
  }

  void OnDatabaseIsReady() override { database_is_ready_ = true; }

  bool did_create_database_ = false;
  bool did_migrate_database_ = false;
  bool failed_to_migrate_database_ = false;
  bool database_is_ready_ = false;
};

TEST_P(BraveAdsDatabaseMigrationTest, MigrateFromSchema) {
  // Act & Assert
  EXPECT_EQ(ShouldCreateDatabase(), did_create_database_);
  EXPECT_EQ(ShouldMigrateDatabase(), did_migrate_database_);
  EXPECT_FALSE(failed_to_migrate_database_);
  EXPECT_TRUE(database_is_ready_);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDatabaseMigrationTest,
                         ::testing::Range(0, database::kVersion + 1),
                         TestParamToString);

}  // namespace brave_ads
