/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_sync/features.h"
#include "components/password_manager/core/browser/features/password_manager_features_util.h"
#include "components/signin/public/base/consent_level.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/test/test_sync_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_password_manager {

// Verifies the Brave override in features_util::IsAccountStorageActive: on
// Android, a syncing user should stay eligible for the account store when the
// flag is off (upstream behavior) and become ineligible when it is on (so
// passwords route to the profile store like desktop).
class BraveAccountStorageFlagTest : public ::testing::Test {
 protected:
  // A signed-in, syncing user with a healthy, active transport.
  syncer::TestSyncService& SyncingUser() {
    // Brave Sync users are legacy sync-feature (kSync) users, so this test must
    // set up that state to exercise the flag path in IsAccountStorageActive.
    // The crbug.com/40066949 deprecation intentionally does not apply here.
    sync_service_.SetSignedIn(signin::ConsentLevel::kSync);  // nocheck
    sync_service_.SetMaxTransportState(
        syncer::SyncService::TransportState::ACTIVE);
    return sync_service_;
  }

  base::test::TaskEnvironment task_environment_;
  syncer::TestSyncService sync_service_;
};

TEST_F(BraveAccountStorageFlagTest, FlagOffKeepsAccountStorageActive) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_TRUE(
      password_manager::features_util::IsAccountStorageActive(&SyncingUser()));
}

TEST_F(BraveAccountStorageFlagTest,
       FlagOnDisablesAccountStorageForSyncingUser) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_FALSE(
      password_manager::features_util::IsAccountStorageActive(&SyncingUser()));
}

}  // namespace brave_password_manager
