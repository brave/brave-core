// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/speedreader_service.h"

#include "brave/components/speedreader/speedreader_pref_migration.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

class SpeedreaderPolicyTest : public testing::Test {
 public:
  SpeedreaderPolicyTest() {
    SpeedreaderService::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  void SetSpeedreaderFeatureEnabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kSpeedreaderPrefFeatureEnabled,
                                 base::Value(value));
  }

  bool IsManaged() {
    return pref_service_.IsManagedPreference(kSpeedreaderPrefFeatureEnabled);
  }

  TestingPrefServiceSimple pref_service_;
};

TEST_F(SpeedreaderPolicyTest, PolicyDisablesSpeedreader) {
  // Initially, feature should be enabled by default and not managed
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
  EXPECT_FALSE(IsManaged());

  // Set policy to disable Speedreader
  SetSpeedreaderFeatureEnabledByPolicy(false);

  // Test that the policy preference is managed and disabled
  EXPECT_TRUE(IsManaged());
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
}

TEST_F(SpeedreaderPolicyTest, PolicyEnablesSpeedreader) {
  // Set policy to explicitly enable Speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);

  // Test that the policy preference is managed and enabled
  EXPECT_TRUE(IsManaged());
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
}

TEST_F(SpeedreaderPolicyTest, DefaultValueWhenNotManaged) {
  // When not managed by policy, feature should be enabled by default
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
  EXPECT_FALSE(IsManaged());
}

TEST_F(SpeedreaderPolicyTest, PolicyChangesAreReflected) {
  // Start with policy enabling speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);
  EXPECT_TRUE(IsSpeedreaderFeatureEnabled(&pref_service_));

  // Change policy to disable speedreader
  SetSpeedreaderFeatureEnabledByPolicy(false);
  EXPECT_FALSE(IsSpeedreaderFeatureEnabled(&pref_service_));

  // Change back to enabling speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);
  EXPECT_TRUE(IsSpeedreaderFeatureEnabled(&pref_service_));
}

TEST_F(SpeedreaderPolicyTest, PolicyWorksWithDefaultsWrite) {
  // This simulates the behavior when using "defaults write" command on macOS
  // where the preference is set but not marked as managed

  // Manually set the preference value without using SetManagedPref
  pref_service_.SetBoolean(kSpeedreaderPrefFeatureEnabled, false);

  // Verify the preference is set but not marked as managed
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
  EXPECT_FALSE(IsManaged());

  // IsSpeedreaderFeatureEnabled should return false since preference is
  // disabled
  EXPECT_FALSE(IsSpeedreaderFeatureEnabled(&pref_service_));
}

class SpeedreaderPrefMigrationTest : public testing::Test {
 public:
  SpeedreaderPrefMigrationTest() {
    SpeedreaderService::RegisterProfilePrefs(pref_service_.registry());
    RegisterProfilePrefsForMigration(pref_service_.registry());
  }

 protected:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(SpeedreaderPrefMigrationTest, MigratesEnabledPrefToNewStructure) {
  // Set up the old preference structure - user had speedreader enabled
  pref_service_.SetBoolean(kSpeedreaderPrefEnabledDeprecated, true);

  // Verify initial state
  EXPECT_TRUE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration results
  EXPECT_FALSE(
      pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));  // Cleared
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Still default (true)
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Migrated to true
}

TEST_F(SpeedreaderPrefMigrationTest, MigratesDisabledPrefToNewStructure) {
  // Set up the old preference structure - user had speedreader disabled
  pref_service_.SetBoolean(kSpeedreaderPrefEnabledDeprecated, false);

  // Verify initial state
  EXPECT_TRUE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration results
  EXPECT_FALSE(
      pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));  // Cleared
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Still default (true)
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Migrated to false
}

TEST_F(SpeedreaderPrefMigrationTest, HandlesNewInstallationWithDefaults) {
  // Simulate new installation - deprecated pref path doesn't exist
  EXPECT_FALSE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));

  // Verify initial defaults
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration does nothing for new installations
  EXPECT_FALSE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderPrefFeatureEnabled));  // Still default
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderPrefEnabledForAllSites));  // Still default
}

TEST_F(SpeedreaderPrefMigrationTest, MigrationIdempotent) {
  // Set up the old preference structure
  pref_service_.SetBoolean(kSpeedreaderPrefEnabledDeprecated, true);

  // Verify the deprecated pref path exists
  EXPECT_TRUE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));

  // Run migration first time
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration worked and deprecated pref is cleared
  EXPECT_FALSE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderPrefEnabledForAllSites));

  // Manually change preferences to test idempotency
  pref_service_.SetBoolean(kSpeedreaderPrefFeatureEnabled, false);
  pref_service_.SetBoolean(kSpeedreaderPrefEnabledForAllSites, false);

  // Run migration again - should do nothing since deprecated pref is gone
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify values weren't changed by second migration
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefFeatureEnabled));
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefEnabledForAllSites));
}

}  // namespace speedreader
