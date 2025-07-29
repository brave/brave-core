// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/speedreader_service.h"

#include "base/test/task_environment.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

class SpeedreaderPolicyTest : public testing::Test {
 public:
  SpeedreaderPolicyTest() {
    SpeedreaderService::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  void SetSpeedreaderDisabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kSpeedreaderDisabledByPolicy,
                                 base::Value(value));
  }

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(SpeedreaderPolicyTest, PolicyDisablesSpeedreader) {
  // Initially, policy should not be disabled
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(pref_service_.IsManagedPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));

  // Set policy to disable Speedreader
  SetSpeedreaderDisabledByPolicy(true);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderDisabledByPolicy));

  // Test that IsDisabledByPolicy function works
  EXPECT_TRUE(IsDisabledByPolicy(&pref_service_));
}

TEST_F(SpeedreaderPolicyTest, PolicyEnablesSpeedreader) {
  // Set policy to explicitly enable Speedreader (policy value false = not
  // disabled)
  SetSpeedreaderDisabledByPolicy(false);

  // Test that the policy preference is set correctly
  EXPECT_TRUE(pref_service_.FindPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_TRUE(pref_service_.IsManagedPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderDisabledByPolicy));

  // Test that IsDisabledByPolicy function works
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));
}

TEST_F(SpeedreaderPolicyTest, DefaultValueWhenNotManaged) {
  // When not managed by policy, should be false by default
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(pref_service_.IsManagedPreference(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));
}

TEST_F(SpeedreaderPolicyTest, PolicyChangesAreReflected) {
  // Start with policy allowing speedreader
  SetSpeedreaderDisabledByPolicy(false);
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));

  // Change policy to disable speedreader
  SetSpeedreaderDisabledByPolicy(true);
  EXPECT_TRUE(IsDisabledByPolicy(&pref_service_));

  // Change back to allowing speedreader
  SetSpeedreaderDisabledByPolicy(false);
  EXPECT_FALSE(IsDisabledByPolicy(&pref_service_));
}

TEST_F(SpeedreaderPolicyTest, PolicyWorksWithDefaultsWrite) {
  // This simulates the behavior when using "defaults write" command on macOS
  // where the preference is set but not marked as managed

  // Manually set the preference value without using SetManagedPref
  pref_service_.SetBoolean(kSpeedreaderDisabledByPolicy, true);

  // Verify the preference is set but not marked as managed
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderDisabledByPolicy));
  EXPECT_FALSE(pref_service_.IsManagedPreference(kSpeedreaderDisabledByPolicy));

  // IsDisabledByPolicy should still return true since we just check the
  // preference value
  EXPECT_TRUE(IsDisabledByPolicy(&pref_service_));
}

}  // namespace speedreader
