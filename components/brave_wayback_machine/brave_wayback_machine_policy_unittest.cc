// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveWaybackMachinePolicyTest : public testing::Test {
 public:
  BraveWaybackMachinePolicyTest() {
    pref_service_.registry()->RegisterBooleanPref(
        kBraveWaybackMachineDisabledByPolicy, false);
  }

 protected:
  void SetWaybackMachineDisabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kBraveWaybackMachineDisabledByPolicy,
                                 base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveWaybackMachinePolicyTest, PolicyDisablesWaybackMachine) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_FALSE(
      pref_service_.IsManagedPreference(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));

  // Set policy to disable Wayback Machine
  SetWaybackMachineDisabledByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(
      pref_service_.FindPreference(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_TRUE(pref_service_.GetBoolean(kBraveWaybackMachineDisabledByPolicy));

  // Test that IsDisabledByPolicy function works
  EXPECT_TRUE(IsDisabledByPolicy(&pref_service_));
}

TEST_F(BraveWaybackMachinePolicyTest, PolicyEnabledExplicitly) {
  // Set policy to enable Wayback Machine explicitly
  SetWaybackMachineDisabledByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(
      pref_service_.FindPreference(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_TRUE(
      pref_service_.IsManagedPreference(kBraveWaybackMachineDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveWaybackMachineDisabledByPolicy));

  // Test that IsDisabledByPolicy function works
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));
}
