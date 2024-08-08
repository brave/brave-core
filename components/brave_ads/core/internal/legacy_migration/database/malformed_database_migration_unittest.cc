/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kMalformedDatabaseSchemaFilename[] =
    "database/migration/malformed_database_schema.sqlite";
}  // namespace

class BraveAdsMalformedDatabaseMigrationTest
    : public test::TestBase,
      public ::testing::WithParamInterface<int>,
      public DatabaseManagerObserver {
 protected:
  void SetUpMocks() override {
    DatabaseManager::GetInstance().AddObserver(this);

    ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
        kMalformedDatabaseSchemaFilename, kDatabaseFilename));
  }

  void TearDown() override {
    DatabaseManager::GetInstance().RemoveObserver(this);

    test::TestBase::TearDown();
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

TEST_F(BraveAdsMalformedDatabaseMigrationTest,
       MigrateFromMalformedDatabaseSchema) {
  // Database migration occurs after invoking `Setup` and `SetUpMocks` during
  // the initialization of `Ads` in `test::TestBase`. Consequently,
  // `EXPECT_CALL` cannot be used with the mocks.

  // Assert
  EXPECT_TRUE(did_create_database_);
  EXPECT_FALSE(did_migrate_database_);
  EXPECT_FALSE(failed_to_migrate_database_);
  EXPECT_TRUE(database_is_ready_);
}

}  // namespace brave_ads
