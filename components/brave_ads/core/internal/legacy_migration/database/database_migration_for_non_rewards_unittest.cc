/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr int kFreshInstallDatabaseVersion = 0;

std::string TestParamToString(::testing::TestParamInfo<int> test_param) {
  return base::StringPrintf("%d_to_%d", test_param.param,
                            database::kVersionNumber);
}

}  // namespace

class BraveAdsDatabaseMigrationForNonRewardsTest
    : public test::TestBase,
      public ::testing::WithParamInterface<int>,
      public DatabaseManagerObserver {
 protected:
  void SetUpMocks() override {
    DatabaseManager::GetInstance().AddObserver(this);

    test::DisableBraveRewards();

    MaybeMockDatabase();
  }

  void TearDown() override {
    DatabaseManager::GetInstance().RemoveObserver(this);

    test::TestBase::TearDown();
  }

  static int GetSchemaVersion() { return GetParam(); }

  static std::string DatabasePathForSchemaVersion() {
    return base::StringPrintf("database/migration/database_schema_%d.sqlite",
                              GetSchemaVersion());
  }

  static bool IsTestingFreshInstall() {
    return GetSchemaVersion() <= database::kRazeDatabaseThresholdVersionNumber;
  }

  static bool IsTestingUpgrade() {
    return !IsTestingFreshInstall() &&
           GetSchemaVersion() > database::kRazeDatabaseThresholdVersionNumber &&
           GetSchemaVersion() < database::kVersionNumber;
  }

  void MaybeMockDatabase() {
    if (IsTestingFreshInstall()) {
      // No need to mock sqlite database for a fresh install.
      return;
    }

    ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
        DatabasePathForSchemaVersion(), kDatabaseFilename));
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

TEST_P(BraveAdsDatabaseMigrationForNonRewardsTest, MigrateFromSchema) {
  // Database migration occurs after invoking `Setup` and `SetUpMocks` during
  // the initialization of `Ads` in `test::TestBase`. Consequently,
  // `EXPECT_CALL` cannot be used with the mocks.

  // Assert
  EXPECT_EQ(IsTestingFreshInstall(), did_create_database_);
  EXPECT_EQ(IsTestingUpgrade(), did_migrate_database_);
  EXPECT_FALSE(failed_to_migrate_database_);
  EXPECT_TRUE(database_is_ready_);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveAdsDatabaseMigrationForNonRewardsTest,
    ::testing::Range(
        kFreshInstallDatabaseVersion,
        database::kVersionNumber +
            1),  // We add 1 because `::testing::Range` end is exclusive.
    TestParamToString);

}  // namespace brave_ads
