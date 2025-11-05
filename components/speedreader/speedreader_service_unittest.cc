// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/speedreader_service.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_pref_migration.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

class SpeedreaderServiceTest : public testing::Test {
 public:
  SpeedreaderServiceTest() : feature_list_(features::kSpeedreaderFeature) {}

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    SpeedreaderService::RegisterProfilePrefs(prefs_.registry());
    SpeedreaderMetrics::RegisterPrefs(prefs_.registry());
    settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, /*is_off_the_record=*/false, /*store_last_modified=*/false,
        /*restore_session=*/false, /*should_record_metrics=*/false);

    user_prefs::UserPrefs::Set(&browser_context_, &prefs_);
    service_ = std::make_unique<SpeedreaderService>(&browser_context_, &prefs_,
                                                    settings_map_.get());
  }

  void TearDown() override { settings_map_->ShutdownOnUIThread(); }

  SpeedreaderService* speedreader_service() { return service_.get(); }

  PrefService* prefs() { return &prefs_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  content::TestBrowserContext browser_context_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  std::unique_ptr<SpeedreaderService> service_;
};

TEST_F(SpeedreaderServiceTest, DefaultSettings) {
  EXPECT_FALSE(speedreader_service()->IsAllowedForAllReadableSites());

  for (const bool enabled : {true, false}) {
    SCOPED_TRACE(testing::Message() << "Enabled: " << enabled);
    speedreader_service()->SetAllowedForAllReadableSites(enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsAllowedForAllReadableSites());
    EXPECT_EQ(enabled,
              prefs()->GetBoolean(kSpeedreaderAllowedForAllReadableSites));
  }
}

TEST_F(SpeedreaderServiceTest, DefaultSiteSettings) {
  const GURL site("https://example.com");

  EXPECT_FALSE(speedreader_service()->IsAllowedForSite(site));
  EXPECT_FALSE(speedreader_service()->IsEnabledForSite(site));
  EXPECT_FALSE(speedreader_service()->IsDisabledForSite(site));
}

TEST_F(SpeedreaderServiceTest, DefaultSiteSettingsAllSitesEnabled) {
  const GURL site("https://example.com");

  speedreader_service()->SetAllowedForAllReadableSites(true);
  EXPECT_TRUE(speedreader_service()->IsAllowedForSite(site));
  EXPECT_FALSE(speedreader_service()->IsEnabledForSite(site));
  EXPECT_FALSE(speedreader_service()->IsDisabledForSite(site));
}

TEST_F(SpeedreaderServiceTest, SpeedreaderDisabled) {
  const GURL site1("https://example.com");
  const GURL site2("https://example2.com");

  prefs()->SetBoolean(kSpeedreaderEnabled, false);
  speedreader_service()->SetEnabledForSite(site2, true);

  for (const bool enabled : {true, false}) {
    SCOPED_TRACE(testing::Message() << "Enabled: " << enabled);
    speedreader_service()->SetAllowedForAllReadableSites(enabled);
    EXPECT_FALSE(speedreader_service()->IsAllowedForSite(site1));
    EXPECT_FALSE(speedreader_service()->IsAllowedForSite(site2));
    EXPECT_FALSE(speedreader_service()->IsEnabledForSite(site1));
    EXPECT_TRUE(speedreader_service()->IsEnabledForSite(site2));
  }
}

TEST_F(SpeedreaderServiceTest, OverrideSiteSettingsAllSitesDefault) {
  const GURL site("https://example.com");

  for (const bool enabled : {true, false}) {
    speedreader_service()->SetEnabledForSite(site, enabled);

    EXPECT_EQ(enabled, speedreader_service()->IsAllowedForSite(site));
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForSite(site));
    EXPECT_EQ(!enabled, speedreader_service()->IsDisabledForSite(site));
  }
}

TEST_F(SpeedreaderServiceTest, OverrideSiteSettingsAllSitesEnabled) {
  const GURL site("https://example.com");

  speedreader_service()->SetAllowedForAllReadableSites(true);
  for (const bool enabled : {true, false}) {
    SCOPED_TRACE(testing::Message() << "Enabled: " << enabled);
    speedreader_service()->SetEnabledForSite(site, enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsAllowedForSite(site));
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForSite(site));
    EXPECT_EQ(!enabled, speedreader_service()->IsDisabledForSite(site));
  }
}

class SpeedreaderPolicyTest : public testing::Test {
 public:
  SpeedreaderPolicyTest() {
    SpeedreaderService::RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  void SetSpeedreaderFeatureEnabledByPolicy(bool value) {
    pref_service_.SetManagedPref(kSpeedreaderEnabled, base::Value(value));
  }

  bool IsManaged() {
    return pref_service_.IsManagedPreference(kSpeedreaderEnabled);
  }

  bool IsSpeedreaderEnabled() {
    return pref_service_.GetBoolean(kSpeedreaderEnabled);
  }

  TestingPrefServiceSimple pref_service_;
};

TEST_F(SpeedreaderPolicyTest, PolicyDisablesSpeedreader) {
  // Initially, feature should be enabled by default and not managed
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderEnabled));
  EXPECT_FALSE(IsManaged());

  // Set policy to disable Speedreader
  SetSpeedreaderFeatureEnabledByPolicy(false);

  // Test that the policy preference is managed and disabled
  EXPECT_TRUE(IsManaged());
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderEnabled));
}

TEST_F(SpeedreaderPolicyTest, PolicyEnablesSpeedreader) {
  // Set policy to explicitly enable Speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);

  // Test that the policy preference is managed and enabled
  EXPECT_TRUE(IsManaged());
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderEnabled));
}

TEST_F(SpeedreaderPolicyTest, DefaultValueWhenNotManaged) {
  // When not managed by policy, feature should be enabled by default
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderEnabled));
  EXPECT_FALSE(IsManaged());
}

TEST_F(SpeedreaderPolicyTest, PolicyChangesAreReflected) {
  // Start with policy enabling speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);
  EXPECT_TRUE(IsSpeedreaderEnabled());

  // Change policy to disable speedreader
  SetSpeedreaderFeatureEnabledByPolicy(false);
  EXPECT_FALSE(IsSpeedreaderEnabled());

  // Change back to enabling speedreader
  SetSpeedreaderFeatureEnabledByPolicy(true);
  EXPECT_TRUE(IsSpeedreaderEnabled());
}

TEST_F(SpeedreaderPolicyTest, PolicyWorksWithDefaultsWrite) {
  // This simulates the behavior when using "defaults write" command on macOS
  // where the preference is set but not marked as managed

  // Manually set the preference value without using SetManagedPref
  pref_service_.SetBoolean(kSpeedreaderEnabled, false);

  // Verify the preference is set but not marked as managed
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderEnabled));
  EXPECT_FALSE(IsManaged());

  // IsSpeedreaderEnabled should return false since preference is
  // disabled
  EXPECT_FALSE(IsSpeedreaderEnabled());
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
  EXPECT_TRUE(
      pref_service_.GetBoolean(kSpeedreaderEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration results
  EXPECT_FALSE(
      pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));  // Cleared
  EXPECT_TRUE(
      pref_service_.GetBoolean(kSpeedreaderEnabled));  // Still default (true)
  EXPECT_TRUE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Migrated to true
}

TEST_F(SpeedreaderPrefMigrationTest, MigratesDisabledPrefToNewStructure) {
  // Set up the old preference structure - user had speedreader disabled
  pref_service_.SetBoolean(kSpeedreaderPrefEnabledDeprecated, false);

  // Verify initial state
  EXPECT_TRUE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(
      pref_service_.GetBoolean(kSpeedreaderEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration results
  EXPECT_FALSE(
      pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));  // Cleared
  EXPECT_TRUE(
      pref_service_.GetBoolean(kSpeedreaderEnabled));  // Still default (true)
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Migrated to false
}

TEST_F(SpeedreaderPrefMigrationTest, HandlesNewInstallationWithDefaults) {
  // Simulate new installation - deprecated pref path doesn't exist
  EXPECT_FALSE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));

  // Verify initial defaults
  EXPECT_TRUE(
      pref_service_.GetBoolean(kSpeedreaderEnabled));  // Default is true
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Default is false

  // Run migration
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify migration does nothing for new installations
  EXPECT_FALSE(pref_service_.HasPrefPath(kSpeedreaderPrefEnabledDeprecated));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderEnabled));  // Still default
  EXPECT_FALSE(pref_service_.GetBoolean(
      kSpeedreaderAllowedForAllReadableSites));  // Still default
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
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderEnabled));
  EXPECT_TRUE(pref_service_.GetBoolean(kSpeedreaderAllowedForAllReadableSites));

  // Manually change preferences to test idempotency
  pref_service_.SetBoolean(kSpeedreaderEnabled, false);
  pref_service_.SetBoolean(kSpeedreaderAllowedForAllReadableSites, false);

  // Run migration again - should do nothing since deprecated pref is gone
  MigrateObsoleteProfilePrefs(&pref_service_);

  // Verify values weren't changed by second migration
  EXPECT_FALSE(pref_service_.GetBoolean(kSpeedreaderEnabled));
  EXPECT_FALSE(
      pref_service_.GetBoolean(kSpeedreaderAllowedForAllReadableSites));
}

}  // namespace speedreader
