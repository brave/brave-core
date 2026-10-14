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

struct TestCase {
  const char* name;
  password_manager::IsAccountStore is_account_store;
  bool feature_enabled;
  syncer::WipeModelUponSyncDisabledBehavior expected;
};

}  // namespace

TEST(BraveWipeModelUponSyncDisabledBehaviorTest,
     GetWipeModelUponSyncDisabledBehavior) {
  // Store          | Feature state | Expected behavior
  // ---------------+---------------+------------------
  // account store  | disabled      | kAlways
  // account store  | enabled       | kNever
  // profile store  | disabled      | kNever
  // profile store  | enabled       | kNever
  const TestCase kCases[] = {
      // Without the feature the account store keeps the upstream behavior: its
      // credentials are the synced ones and go away with the sync chain.
      {"account store, feature disabled", kAccountStore, false,
       syncer::WipeModelUponSyncDisabledBehavior::kAlways},
      // With the feature the profile store is the synced one, so account-store
      // credentials have to survive leaving the sync chain for the startup
      // migrator to move them.
      {"account store, feature enabled", kAccountStore, true,
       syncer::WipeModelUponSyncDisabledBehavior::kNever},
      // The profile store holds the local credentials and is never wiped,
      // including when it becomes the synced store.
      {"profile store, feature disabled", kProfileStore, false,
       syncer::WipeModelUponSyncDisabledBehavior::kNever},
      {"profile store, feature enabled", kProfileStore, true,
       syncer::WipeModelUponSyncDisabledBehavior::kNever},
  };

  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.name);
    base::test::ScopedFeatureList features;
    features.InitWithFeatureState(
        brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore,
        test_case.feature_enabled);

    EXPECT_EQ(test_case.expected,
              GetWipeModelUponSyncDisabledBehavior(test_case.is_account_store));
  }
}

}  // namespace brave_password_manager
