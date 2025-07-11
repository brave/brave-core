// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveTalkPolicyTest : public testing::Test {
 public:
  BraveTalkPolicyTest() {
    // Register the Brave Talk policy preference
    pref_service_.registry()->RegisterBooleanPref(kBraveTalkDisabledByPolicy,
                                                  false);
  }

 protected:
  void SetBraveTalkDisabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kBraveTalkDisabledByPolicy,
                                 base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveTalkPolicyTest, PolicyDisablesBraveTalk) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
  EXPECT_FALSE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));

  // Set policy to disable Brave Talk
  SetBraveTalkDisabledByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(kBraveTalkDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));
  EXPECT_TRUE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, PolicyEnablesBraveTalk) {
  // Set policy to explicitly enable Brave Talk
  SetBraveTalkDisabledByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(kBraveTalkDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, PolicyChangesAreReflected) {
  // Start with policy disabled
  SetBraveTalkDisabledByPolicy(false);
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));

  // Change policy to enabled
  SetBraveTalkDisabledByPolicy(true);
  EXPECT_TRUE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, DefaultValueWhenNotManaged) {
  // When not managed by policy, should be false by default
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveTalkDisabledByPolicy));
  EXPECT_FALSE(pref_service_.IsManagedPreference(kBraveTalkDisabledByPolicy));
}
