/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_wipe_model_upon_sync_disabled_behavior.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_sync/features.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/sync/model/wipe_model_upon_sync_disabled_behavior.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_password_manager {

namespace {

constexpr password_manager::IsAccountStore kAccountStore(true);
constexpr password_manager::IsAccountStore kProfileStore(false);

}  // namespace

// Without the feature the account store keeps the upstream behavior: its
// credentials are the synced ones and go away with the sync chain.
TEST(BraveWipeModelUponSyncDisabledBehaviorTest, AccountStoreFeatureDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_EQ(syncer::WipeModelUponSyncDisabledBehavior::kAlways,
            GetWipeModelUponSyncDisabledBehavior(kAccountStore));
}

// With the feature the profile store is the synced one, so account-store
// credentials have to survive leaving the sync chain for the startup migrator
// to move them.
TEST(BraveWipeModelUponSyncDisabledBehaviorTest, AccountStoreFeatureEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_EQ(syncer::WipeModelUponSyncDisabledBehavior::kNever,
            GetWipeModelUponSyncDisabledBehavior(kAccountStore));
}

TEST(BraveWipeModelUponSyncDisabledBehaviorTest, ProfileStoreFeatureDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_EQ(syncer::WipeModelUponSyncDisabledBehavior::kNever,
            GetWipeModelUponSyncDisabledBehavior(kProfileStore));
}

// The profile store holds the local credentials and is never wiped, including
// when it becomes the synced store.
TEST(BraveWipeModelUponSyncDisabledBehaviorTest, ProfileStoreFeatureEnabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(
      brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);

  EXPECT_EQ(syncer::WipeModelUponSyncDisabledBehavior::kNever,
            GetWipeModelUponSyncDisabledBehavior(kProfileStore));
}

}  // namespace brave_password_manager
