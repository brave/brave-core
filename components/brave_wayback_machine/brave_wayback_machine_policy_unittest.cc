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
    pref_service_.registry()->RegisterBooleanPref(kBraveWaybackMachineEnabled,
                                                  true);
  }

 protected:
  void SetWaybackMachineEnabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kBraveWaybackMachineEnabled,
                                 base::Value(value));
  }

  bool IsManaged() {
    return pref_service_.IsManagedPreference(kBraveWaybackMachineEnabled);
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveWaybackMachinePolicyTest, PolicyDisablesWaybackMachine) {
  // Initially, policy should not be managed
  EXPECT_TRUE(pref_service_.GetBoolean(kBraveWaybackMachineEnabled));
  EXPECT_FALSE(IsManaged());

  // Set policy to disable Wayback Machine
  SetWaybackMachineEnabledByPolicy(false);

  // Test that the policy preference is managed and disabled
  EXPECT_TRUE(IsManaged());
  EXPECT_FALSE(pref_service_.GetBoolean(kBraveWaybackMachineEnabled));
}

TEST_F(BraveWaybackMachinePolicyTest, PolicyEnabledExplicitly) {
  // Set policy to enable Wayback Machine explicitly
  SetWaybackMachineEnabledByPolicy(true);

  // Test that the policy preference is managed and enabled
  EXPECT_TRUE(IsManaged());
  EXPECT_TRUE(pref_service_.GetBoolean(kBraveWaybackMachineEnabled));
}
