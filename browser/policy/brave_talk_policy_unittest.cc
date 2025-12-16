// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_talk/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveTalkPolicyTest : public testing::Test {
 public:
  BraveTalkPolicyTest() {
    // Register the Brave Talk policy preference
    pref_service_.registry()->RegisterBooleanPref(
        brave_talk::prefs::kDisabledByPolicy, false);
  }

 protected:
  void SetBraveTalkDisabledByPolicy(bool value) {
    pref_service_.SetManagedPref(brave_talk::prefs::kDisabledByPolicy,
                                 base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveTalkPolicyTest, PolicyDisablesBraveTalk) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));

  // Set policy to disable Brave Talk
  SetBraveTalkDisabledByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(
      pref_service_.FindPreference(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_TRUE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, PolicyEnablesBraveTalk) {
  // Set policy to explicitly enable Brave Talk
  SetBraveTalkDisabledByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(
      pref_service_.FindPreference(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, PolicyChangesAreReflected) {
  // Start with policy disabled
  SetBraveTalkDisabledByPolicy(false);
  EXPECT_FALSE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));

  // Change policy to enabled
  SetBraveTalkDisabledByPolicy(true);
  EXPECT_TRUE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));
}

TEST_F(BraveTalkPolicyTest, DefaultValueWhenNotManaged) {
  // When not managed by policy, should be false by default
  EXPECT_FALSE(pref_service_.GetBoolean(brave_talk::prefs::kDisabledByPolicy));
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(brave_talk::prefs::kDisabledByPolicy));
}
