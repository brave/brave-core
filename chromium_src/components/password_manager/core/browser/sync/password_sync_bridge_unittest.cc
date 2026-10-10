/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_sync/features.h"
#include "build/build_config.h"

#include <components/password_manager/core/browser/sync/password_sync_bridge_unittest.cc>

#if BUILDFLAG(IS_ANDROID)

namespace password_manager {
namespace {

// With kBraveAndroidSyncPasswordsInProfileStore on, the backend factory builds
// the account store with kNever, because the startup migrator is what drains it
// into the profile store. Leaving the sync chain must then keep the credentials
// and only drop the sync metadata. This combination also exercises the relaxed
// CHECK(!IsAccountStore()) on the kNever branch.
TEST_F(PasswordSyncBridgeTest, BraveKeepsAccountCredentialsOnSyncDisable) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  ON_CALL(*mock_password_store_sync(), IsAccountStore())
      .WillByDefault(Return(true));
  fake_db()->AddLoginWithPrimaryKey(MakeStoredCredential(kSignonRealm1));

  EXPECT_CALL(*mock_sync_metadata_store_sync(),
              DeleteAllSyncMetadata(syncer::PASSWORDS));
  EXPECT_CALL(*mock_password_store_sync(), DeleteAndRecreateDatabaseFile())
      .Times(0);
  EXPECT_CALL(*mock_password_store_sync(), NotifyCredentialsChanged).Times(0);
  EXPECT_CALL(*mock_sync_enabled_or_disabled_cb(), Run());

  bridge()->ApplyDisableSyncChanges(bridge()->CreateMetadataChangeList());
}

// With the flag off the factory keeps picking kAlways for the account store,
// which must still wipe both data and metadata and report the removals. The
// flag is irrelevant to this path, so it is left at its default.
TEST_F(PasswordSyncBridgeAccountStoreTest,
       BraveWipesAccountCredentialsOnSyncDisable) {
  fake_db()->AddLoginWithPrimaryKey(MakeStoredCredential(kSignonRealm1));

  EXPECT_CALL(*mock_sync_metadata_store_sync(),
              DeleteAllSyncMetadata(syncer::PASSWORDS));
  EXPECT_CALL(*mock_password_store_sync(), DeleteAndRecreateDatabaseFile());
  EXPECT_CALL(*mock_password_store_sync(),
              NotifyCredentialsChanged(testing::SizeIs(1)));

  bridge()->ApplyDisableSyncChanges(bridge()->CreateMetadataChangeList());
}

}  // namespace
}  // namespace password_manager

#endif  // BUILDFLAG(IS_ANDROID)
