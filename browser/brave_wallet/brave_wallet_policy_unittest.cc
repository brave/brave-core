// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletPolicyTest : public testing::Test {
 public:
  BraveWalletPolicyTest() {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
  }

 protected:
  void BlockWalletByPolicy(bool value) {
    prefs_.SetManagedPref(prefs::kDisabledByPolicy, base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(BraveWalletPolicyTest, PolicyDisablesWallet) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kDisabledByPolicy));

  // Set policy to disable Brave Wallet
  BlockWalletByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kDisabledByPolicy));

  // Test that IsAllowed returns false when policy disables it
  EXPECT_FALSE(IsAllowed(&prefs_));
}

TEST_F(BraveWalletPolicyTest, PolicyEnablesWallet) {
  // Set policy to enable Brave Wallet
  BlockWalletByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kDisabledByPolicy));

  // Test that IsAllowed returns true when policy enables it
  EXPECT_TRUE(IsAllowed(&prefs_));
}

TEST_F(BraveWalletPolicyTest, DefaultBehavior) {
  // Test that IsAllowed returns true when no policy is set
  EXPECT_TRUE(IsAllowed(&prefs_));
}

}  // namespace brave_wallet
