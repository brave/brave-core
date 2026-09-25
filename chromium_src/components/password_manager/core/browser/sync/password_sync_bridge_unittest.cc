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

// With kBraveAndroidSyncPasswordsInProfileStore on, leaving the sync chain must
// keep the account-store credentials, because the startup migrator is what
// drains them into the profile store. Only the sync metadata may be dropped.
// Reaching the kNever branch with an account store also exercises the relaxed
// CHECK(!IsAccountStore()) there.
TEST_F(PasswordSyncBridgeAccountStoreTest,
       BraveKeepsAccountCredentialsOnSyncDisable) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  fake_db()->AddLoginWithPrimaryKey(MakeStoredCredential(kSignonRealm1));

  EXPECT_CALL(*mock_sync_metadata_store_sync(),
              DeleteAllSyncMetadata(syncer::PASSWORDS));
  EXPECT_CALL(*mock_password_store_sync(), DeleteAndRecreateDatabaseFile())
      .Times(0);
  EXPECT_CALL(*mock_password_store_sync(), NotifyCredentialsChanged).Times(0);
  EXPECT_CALL(*mock_sync_enabled_or_disabled_cb(), Run());

  bridge()->ApplyDisableSyncChanges(bridge()->CreateMetadataChangeList());
}

// Without the flag the account store keeps the upstream behavior: data and
// metadata are both wiped and the removals are reported to the store.
TEST_F(PasswordSyncBridgeAccountStoreTest,
       BraveWipesAccountCredentialsOnSyncDisableWithoutFlag) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
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
