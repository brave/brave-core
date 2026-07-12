// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"

#include <array>
#include <tuple>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave_shields::mojom::AdBlockMode;
using brave_shields::mojom::AutoShredMode;
using brave_shields::mojom::FingerprintMode;

class BraveShieldsSettingsServiceTest : public testing::Test {
 public:
  BraveShieldsSettingsServiceTest() {}
  ~BraveShieldsSettingsServiceTest() override = default;

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    brave_shields::RegisterShieldsP3AProfilePrefs(profile_prefs_.registry());
    brave_shields::RegisterShieldsP3ALocalPrefs(local_state_.registry());
    host_content_settings_map_ = new HostContentSettingsMap(
        &profile_prefs_, false /* is_off_the_record */,
        false /* store_last_modified */, false /* restore_session */,
        false /* should_record_metrics */);
    brave_shields_settings_ =
        std::make_unique<brave_shields::BraveShieldsSettingsService>(
            *GetHostContentSettingsMap(), GetLocalState(), &profile_prefs_);
  }

  void TearDown() override { host_content_settings_map_->ShutdownOnUIThread(); }

  TestingPrefServiceSimple* GetLocalState() { return &local_state_; }
  HostContentSettingsMap* GetHostContentSettingsMap() {
    return host_content_settings_map_.get();
  }

  const GURL kTestUrl{"https://brave.com"};

  brave_shields::BraveShieldsSettingsService* brave_shields_settings() {
    return brave_shields_settings_.get();
  }

  base::Value AutoShredDictFrom(AutoShredMode mode) {
    base::Value dict(base::Value::Type::DICT);
    dict.GetDict().Set(brave_shields::AutoShredSetting::kName,
                       static_cast<int>(mode));
    return dict;
  }

 private:
  // These tests are part of the "brave_components_unittests" target and the
  // test runner inherits from base::TestSuite rather than ChromeTestSuite.
  // The latter is the one that automatically adds this seed for tests.
  // See chrome_test_suite.h for the equivalent seed.
  brave_shields::ScopedStableFarblingTokensForTesting stable_farbling_seed_{1};
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettingsService>
      brave_shields_settings_;
};

TEST_F(BraveShieldsSettingsServiceTest, BraveShieldsEnabled) {
  // verify the initial values
  EXPECT_TRUE(brave_shields_settings()->GetBraveShieldsEnabled(kTestUrl));
  EXPECT_TRUE(brave_shields::GetBraveShieldsEnabled(GetHostContentSettingsMap(),
                                                    kTestUrl));

  brave_shields_settings()->SetBraveShieldsEnabled(false, kTestUrl);
  EXPECT_FALSE(brave_shields_settings()->GetBraveShieldsEnabled(kTestUrl));
  // verify underlying value GetBraveShieldsEnabled is updated
  EXPECT_FALSE(brave_shields::GetBraveShieldsEnabled(
      GetHostContentSettingsMap(), kTestUrl));

  // verify other urls unchanged
  EXPECT_TRUE(brave_shields_settings()->GetBraveShieldsEnabled(
      GURL("https://example.com")));
  EXPECT_TRUE(brave_shields::GetBraveShieldsEnabled(
      GetHostContentSettingsMap(), GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsServiceTest, AdBlockMode) {
  // verify the initial values
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::STANDARD);
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), kTestUrl),
      brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields_settings()->SetAdBlockMode(AdBlockMode::AGGRESSIVE, kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::AGGRESSIVE);
  // verify underlying AdControlType & CosmeticFilteringControlType is updated
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), kTestUrl),
      brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::BLOCK);

  brave_shields_settings()->SetAdBlockMode(AdBlockMode::ALLOW, kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::ALLOW);
  // verify underlying AdControlType & CosmeticFilteringControlType is updated
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), kTestUrl),
      brave_shields::ControlType::ALLOW);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::ALLOW);

  // verify other urls remain unchanged
  EXPECT_EQ(
      brave_shields_settings()->GetAdBlockMode(GURL("https://example.com")),
      AdBlockMode::STANDARD);
}

TEST_F(BraveShieldsSettingsServiceTest, DefaultAdBlockMode) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetAdBlockMode(AdBlockMode::STANDARD, kTestUrl);

  // verify the initial default values
  EXPECT_EQ(brave_shields_settings()->GetDefaultAdBlockMode(),
            AdBlockMode::STANDARD);
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), GURL()),
      brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), GURL()),
            brave_shields::ControlType::BLOCK_THIRD_PARTY);

  brave_shields_settings()->SetDefaultAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(brave_shields_settings()->GetDefaultAdBlockMode(),
            AdBlockMode::AGGRESSIVE);
  // verify underlying AdControlType & CosmeticFilteringControlType is updated
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), GURL()),
      brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), GURL()),
            brave_shields::ControlType::BLOCK);
  // verify defaults apply to all
  EXPECT_EQ(
      brave_shields_settings()->GetAdBlockMode(GURL("https://example.com")),
      AdBlockMode::AGGRESSIVE);
  // verify underlying AdControlType & CosmeticFilteringControlType is updated
  EXPECT_EQ(brave_shields::GetAdControlType(GetHostContentSettingsMap(),
                                            GURL("https://example.com")),
            brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), GURL("https://example.com")),
            brave_shields::ControlType::BLOCK);

  // verify explict set adblock mode is unchanged
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::STANDARD);
  // verify underlying AdControlType & CosmeticFilteringControlType is unchanged
  EXPECT_EQ(
      brave_shields::GetAdControlType(GetHostContentSettingsMap(), kTestUrl),
      brave_shields::ControlType::BLOCK);
  EXPECT_EQ(brave_shields::GetCosmeticFilteringControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::BLOCK_THIRD_PARTY);
}

TEST_F(BraveShieldsSettingsServiceTest, FingerprintMode) {
  // verify the initial values
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::DEFAULT);

  brave_shields_settings()->SetFingerprintMode(FingerprintMode::ALLOW_MODE,
                                               kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::ALLOW_MODE);
  // verify underlying FingerprintingControlType is updated
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::ALLOW);

  // iOS does not support FingerprintMode::STRICT_MODE
#if !BUILDFLAG(IS_IOS)
  // when kBraveShowStrictFingerprintingMode flag is disabled
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STRICT_MODE,
                                               kTestUrl);
  // verify it returns FingerprintMode::STANDARD_MODE
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
  // verify underlying FingerprintingControlType is updated
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::DEFAULT);

  // enable kBraveShowStrictFingerprintingMode flag
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_shields::features::kBraveShowStrictFingerprintingMode);
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STRICT_MODE,
                                               kTestUrl);
  // verify it returns FingerprintMode::STRICT_MODE
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STRICT_MODE);
  // verify underlying FingerprintingControlType is updated
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::BLOCK);
#endif

  // verify other urls remain unchanged
  EXPECT_EQ(
      brave_shields_settings()->GetFingerprintMode(GURL("https://example.com")),
      FingerprintMode::STANDARD_MODE);
  // verify underlying FingerprintingControlType is unchanged
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), GURL("https://example.com")),
            brave_shields::ControlType::DEFAULT);
}

TEST_F(BraveShieldsSettingsServiceTest, DefaultFingerprintMode) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STANDARD_MODE,
                                               kTestUrl);

  // verify the initial default values
  EXPECT_EQ(brave_shields_settings()->GetDefaultFingerprintMode(),
            FingerprintMode::STANDARD_MODE);
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), GURL()),
            brave_shields::ControlType::DEFAULT);

  brave_shields_settings()->SetDefaultFingerprintMode(
      FingerprintMode::ALLOW_MODE);
  EXPECT_EQ(brave_shields_settings()->GetDefaultFingerprintMode(),
            FingerprintMode::ALLOW_MODE);
  // verify underlying FingerprintingControlType is updated
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), GURL()),
            brave_shields::ControlType::ALLOW);

  EXPECT_EQ(
      brave_shields_settings()->GetFingerprintMode(GURL("https://example.com")),
      FingerprintMode::ALLOW_MODE);

  // verify explict set fingerprint mode is unchanged
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
  // verify underlying FingerprintingControlType is unchanged
  EXPECT_EQ(brave_shields::GetFingerprintingControlType(
                GetHostContentSettingsMap(), kTestUrl),
            brave_shields::ControlType::DEFAULT);
}

TEST_F(BraveShieldsSettingsServiceTest, NoScriptsEnabled) {
  // verify the initial values
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  kTestUrl),
            brave_shields::ControlType::ALLOW);

  brave_shields_settings()->SetNoScriptEnabled(true, kTestUrl);
  EXPECT_TRUE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));
  // verify underlying NoScriptControlType is updated
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  kTestUrl),
            brave_shields::ControlType::BLOCK);

  // verify other urls remain unchanged
  EXPECT_FALSE(
      brave_shields_settings()->IsNoScriptEnabled(GURL("https://example.com")));
  // verify underlying NoScriptControlType is unchanged
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  GURL("https://example.com")),
            brave_shields::ControlType::ALLOW);
}

TEST_F(BraveShieldsSettingsServiceTest, NoScriptsEnabledByDefault) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetNoScriptEnabled(false, kTestUrl);

  // verify the initial default values
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabledByDefault());
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  GURL()),
            brave_shields::ControlType::ALLOW);

  brave_shields_settings()->SetNoScriptEnabledByDefault(true);
  EXPECT_TRUE(brave_shields_settings()->IsNoScriptEnabledByDefault());
  EXPECT_TRUE(
      brave_shields_settings()->IsNoScriptEnabled(GURL("https://example.com")));
  // verify underlying NoScriptControlType is updated
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  GURL()),
            brave_shields::ControlType::BLOCK);

  // verify explict set no script enabled setting is unchanged
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));
  // verify underlying NoScriptControlType is unchanged
  EXPECT_EQ(brave_shields::GetNoScriptControlType(GetHostContentSettingsMap(),
                                                  kTestUrl),
            brave_shields::ControlType::ALLOW);
}

class BraveShieldsSettingsServiceShredFeatureTest
    : public BraveShieldsSettingsServiceTest {
 public:
  BraveShieldsSettingsServiceShredFeatureTest() {
    scoped_feature_list_.InitAndEnableFeature(
        brave_shields::features::kBraveShredFeature);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveShieldsSettingsServiceShredFeatureTest, AutoShredMode) {
  // verify the initial values
  EXPECT_EQ(brave_shields_settings()->GetAutoShredMode(kTestUrl),
            AutoShredMode::NEVER);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                kTestUrl, GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::NEVER));

  brave_shields_settings()->SetAutoShredMode(AutoShredMode::LAST_TAB_CLOSED,
                                             kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAutoShredMode(kTestUrl),
            AutoShredMode::LAST_TAB_CLOSED);
  // verify underlying AutoShredMode is updated
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                kTestUrl, GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::LAST_TAB_CLOSED));

  brave_shields_settings()->SetAutoShredMode(AutoShredMode::APP_EXIT, kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAutoShredMode(kTestUrl),
            AutoShredMode::APP_EXIT);
  // verify underlying AutoShredMode is updated
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                kTestUrl, GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::APP_EXIT));

  // verify other urls remain unchanged
  EXPECT_EQ(
      brave_shields_settings()->GetAutoShredMode(GURL("https://example.com")),
      AutoShredMode::NEVER);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                GURL("https://example.com"), GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::NEVER));
}

TEST_F(BraveShieldsSettingsServiceShredFeatureTest, DefaultAutoShredMode) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetAutoShredMode(AutoShredMode::NEVER, kTestUrl);

  // verify the initial default values
  EXPECT_EQ(brave_shields_settings()->GetDefaultAutoShredMode(),
            AutoShredMode::NEVER);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                GURL(), GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::NEVER));

  brave_shields_settings()->SetDefaultAutoShredMode(
      AutoShredMode::LAST_TAB_CLOSED);
  EXPECT_EQ(brave_shields_settings()->GetDefaultAutoShredMode(),
            AutoShredMode::LAST_TAB_CLOSED);
  EXPECT_EQ(
      brave_shields_settings()->GetAutoShredMode(GURL("https://example.com")),
      AutoShredMode::LAST_TAB_CLOSED);
  // verify underlying AutoShredMode is updated
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                GURL(), GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::LAST_TAB_CLOSED));

  // verify explict set auto shred mode unchanged
  EXPECT_EQ(brave_shields_settings()->GetAutoShredMode(kTestUrl),
            AutoShredMode::NEVER);
  // verify underlying AutoShredMode is unchanged
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                kTestUrl, GURL(),
                brave_shields::AutoShredSetting::kContentSettingsType),
            AutoShredDictFrom(AutoShredMode::NEVER));
}

#if !BUILDFLAG(IS_IOS)
TEST_F(BraveShieldsSettingsServiceTest, DefaultForgetFirstPartyStorage) {
  // verify the initial values
  EXPECT_EQ(brave_shields_settings()->GetForgetFirstPartyStorageEnabled(GURL()),
            false);
  EXPECT_EQ(
      brave_shields_settings()->GetForgetFirstPartyStorageEnabled(kTestUrl),
      false);
}

TEST_F(BraveShieldsSettingsServiceTest, DefaultForgetFirstPartyStorageEnabled) {
  // verify the initial default values
  EXPECT_EQ(brave_shields_settings()->GetForgetFirstPartyStorageEnabled(GURL()),
            false);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                GURL(), GURL(), ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE),
            CONTENT_SETTING_ALLOW);
}

TEST_F(BraveShieldsSettingsServiceTest,
       SetForgetFirstPartyStorageEnabledUsesETLD) {
  const GURL test_url("https://brave.com");
  const GURL test_subdomain_url("https://www.brave.com");

  EXPECT_EQ(
      GetHostContentSettingsMap()->GetWebsiteSetting(
          test_url, GURL(), ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE),
      CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                test_subdomain_url, GURL(),
                ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE),
            CONTENT_SETTING_ALLOW);

  brave_shields_settings()->SetForgetFirstPartyStorageEnabled(
      true, test_subdomain_url);

  EXPECT_EQ(
      GetHostContentSettingsMap()->GetWebsiteSetting(
          test_url, GURL(), ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE),
      CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetHostContentSettingsMap()->GetWebsiteSetting(
                test_subdomain_url, GURL(),
                ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE),
            CONTENT_SETTING_BLOCK);
}
#endif

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_NoSettings) {
  // With no shield settings, should return false
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_ExactDomainMatch) {
  // Tests exact domain matching
  const auto example_com_url = GURL("https://example.com");
  brave_shields_settings()->SetBraveShieldsEnabled(false, example_com_url);
  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_WildcardPattern) {
  // Set shields to BLOCK for wildcard pattern
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("[*.]example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_SubdomainPattern) {
  // Tests specific subdomain patterns
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("sub.example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_AllowSettingIgnored) {
  // Set shields to ALLOW (not BLOCK) - should be ignored
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_ALLOW);

  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_AllHostsPatternIgnored) {
  // Test whildcard hosts pattern
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SHIELDS, CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_MultipleSettings) {
  // Set multiple settings - some matching, some not
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("test.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("other.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_ALLOW);

  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("[*.]example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://other.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://unrelated.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_DifferentSubdomains) {
  // Set shields to BLOCK for specific subdomain
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("mail.test.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  // Should match for test.com ephemeral domain
  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_PortsInPatterns) {
  // Test with patterns that include ports
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("example.com:8080"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com:8080")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_MixedSchemePatterns) {
  // Test with different scheme patterns
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("https://test.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://test.com")));
}

TEST_F(BraveShieldsSettingsServiceTest,
       IsShieldsDisabledOnAnyHostMatchingDomainOf_DeepSubdomainHierarchy) {
  // Test with deeply nested subdomains
  GetHostContentSettingsMap()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("a.b.c.d.example.com"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://example.com")));
  EXPECT_FALSE(
      brave_shields_settings()->IsShieldsDisabledOnAnyHostMatchingDomainOf(
          GURL("https://other.com")));
}

TEST_F(BraveShieldsSettingsServiceTest, PRNGKnownValues) {
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 10450951993123491723UL),
      std::make_tuple<>(GURL("http://b.com"), 2581208260237394178UL),
  };
  for (const auto& c : test_cases) {
    brave_shields::FarblingPRNG prng;
    ASSERT_TRUE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), {}, &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveShieldsSettingsServiceTest, PRNGKnownValuesDifferentSeeds) {
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 10450951993123491723UL),
      std::make_tuple<>(GURL("http://b.com"), 2581208260237394178UL),
  };
  for (const auto& c : test_cases) {
    brave_shields::FarblingPRNG prng;
    ASSERT_TRUE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), {}, &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveShieldsSettingsServiceTest, InvalidDomains) {
  const std::array<GURL, 8> test_cases = {
      GURL("about:blank"),
      GURL("brave://settings"),
      GURL("chrome://version"),
      GURL("file:///etc/passwd"),
      GURL("javascript:alert(1)"),
      GURL("data:text/plain;base64,"),
      GURL(""),
  };
  for (const auto& url : test_cases) {
    brave_shields::FarblingPRNG prng;
    EXPECT_FALSE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
        url, {}, &prng));
    EXPECT_FALSE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
        url, {}, &prng));
  }
}

TEST_F(BraveShieldsSettingsServiceTest, ShieldsDown) {
  const GURL url("http://a.com");
  brave_shields::SetBraveShieldsEnabled(GetHostContentSettingsMap(), false,
                                        url);
  brave_shields::FarblingPRNG prng;
  EXPECT_FALSE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
      url, {}, &prng));
}

TEST_F(BraveShieldsSettingsServiceTest, FingerprintingAllowed) {
  const GURL url("http://a.com");
  brave_shields::SetFingerprintingControlType(
      GetHostContentSettingsMap(), brave_shields::ControlType::ALLOW, url);
  brave_shields::FarblingPRNG prng;
  EXPECT_FALSE(brave_shields_settings()->MakePseudoRandomGeneratorForURL(
      url, {}, &prng));
}

// Farbling token related tests.

class BraveShieldsSettingsFarblingTest
    : public BraveShieldsSettingsServiceTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveShieldsSettingsFarblingTest() {
    if (GetParam()) {
      scoped_feature_list_.InitWithFeatures(
          {brave_shields::features::kBraveFarblingTokenReset}, {});
    } else {
      scoped_feature_list_.InitWithFeatures(
          {}, {brave_shields::features::kBraveFarblingTokenReset});
    }
  }

  bool IsFarblingTokenResetEnabled() const { return GetParam(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveShieldsSettingsFarblingTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param
                 ? "BraveShieldsSettingsFarblingTest_FarblingTokenResetEnabled"
                 : "BraveShieldsSettingsFarblingTest_"
                   "FarblingTokenResetDisabled";
    });

// Unsupported schemes (chrome://, about:blank, data:, invalid URL) must return
// the zero token because shields are not active there.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_UnsupportedScheme_ReturnsZeroToken) {
  const base::Token zero;
  EXPECT_EQ(zero, brave_shields_settings()->GetFarblingToken(
                      GURL("chrome://settings"), {}));
  EXPECT_EQ(zero, brave_shields_settings()->GetFarblingToken(
                      GURL("about:blank"), {}));
  EXPECT_EQ(zero, brave_shields_settings()->GetFarblingToken(
                      GURL("data:text/html,<h1>hello</h1>"), {}));
  EXPECT_EQ(zero, brave_shields_settings()->GetFarblingToken(
                      GURL("file:///etc/hosts"), {}));
  EXPECT_EQ(zero, brave_shields_settings()->GetFarblingToken(GURL(), {}));
}

// HTTP and HTTPS URLs must produce a non-zero token.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_HttpAndHttps_ReturnNonZeroToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  EXPECT_FALSE(brave_shields_settings()
                   ->GetFarblingToken(GURL("http://example.com"), {})
                   .is_zero());
  EXPECT_FALSE(brave_shields_settings()
                   ->GetFarblingToken(GURL("https://example.com"), {})
                   .is_zero());
}

// Two URLs with the same origin but different paths must share the same token,
// because the path is stripped when deriving the effective URL.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_SameOrigin_DifferentPaths_SameToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto t1 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com/page1"), {});
  const auto t2 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com/page2?q=1#anchor"), {});
  EXPECT_EQ(t1, t2);
}

// A blob URL whose inner origin is https://example.com must yield the same
// token as a plain https://example.com URL, because both resolve to the same
// effective origin (https://example.com/).
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_BlobUrl_SameTokenAsOriginUrl) {
  // Use a random seed (0 = random) to exercise the storage path: the first
  // caller writes a random token keyed under https://example.com/; the second
  // caller reads it back regardless of whether it used the blob or plain URL.
  const auto blob_token = brave_shields_settings()->GetFarblingToken(
      url::Origin::Create(
          GURL("blob:https://example.com/550e8400-e29b-41d4-a716-446655440000"))
          .GetURL(),
      {});
  const auto https_token = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com/some/path"), {});
  EXPECT_EQ(blob_token, https_token);
}

// A blob URL whose inner origin is a subdomain must resolve to the same token
// as a plain HTTPS URL for that subdomain (the subdomain itself shares a token
// with the root via schemeful-site scoping — see
// FarblingToken_SubdomainAndRoot_ShareToken).
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsSubdomainUrl) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  const auto sub_token = brave_shields_settings()->GetFarblingToken(
      GURL("https://sub.example.com"), {});
  const auto blob_sub_token = brave_shields_settings()->GetFarblingToken(
      url::Origin::Create(GURL("blob:https://sub.example.com/some-uuid"))
          .GetURL(),
      {});
  EXPECT_EQ(sub_token, blob_sub_token);
}

// A blob URL for a subdomain must share the token of the root domain.
// BRAVE_SHIELDS_METADATA is registered with
// REQUESTING_SCHEMEFUL_SITE_ONLY_SCOPE, so the content setting is keyed by the
// schemeful site (eTLD+1 + scheme). https://example.com and
// https://sub.example.com resolve to the same schemeful site, so they always
// share one token.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsRootDomain) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  const auto root_token = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  const auto blob_sub_token = brave_shields_settings()->GetFarblingToken(
      url::Origin::Create(GURL("blob:https://sub.example.com/some-uuid"))
          .GetURL(),
      {});
  EXPECT_EQ(root_token, blob_sub_token);
}

// Two completely different origins must get different tokens.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_DifferentOrigins_DifferentTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  const auto t1 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  const auto t2 =
      brave_shields_settings()->GetFarblingToken(GURL("https://other.com"), {});
  EXPECT_NE(t1, t2);
}

// A subdomain and its root domain share the same token as they map to same
// schemeful site (eTLD+1 + scheme).
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_SubdomainAndRoot_ShareToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  const auto root = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  const auto sub = brave_shields_settings()->GetFarblingToken(
      GURL("https://sub.example.com"), {});
  EXPECT_EQ(root, sub);
}

// Calling GetFarblingToken twice for the same URL must return the same token.
// This verifies that the token is persisted in HostContentSettingsMap and not
// regenerated on every call.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_IsStableAcrossMultipleCalls) {
  const auto t1 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  const auto t2 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  EXPECT_EQ(t1, t2);
  EXPECT_FALSE(t1.is_zero());
}

// Providing additional_entropy must produce a token different from the base
// token for the same URL.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_AdditionalEntropy_ChangesToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  const auto base_token = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), {});
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_token = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  EXPECT_NE(base_token, derived_token);
}

// Two calls with the same URL and the same additional_entropy must produce the
// same derived token (the XOR derivation is deterministic given a stable base).
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_SameEntropy_ProducesSameDerivedToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  constexpr std::array<uint8_t, 4> entropy = {0x05, 0x06, 0x07, 0x08};
  const auto t1 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  const auto t2 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  EXPECT_EQ(t1, t2);
}

// Two calls with the same URL but different additional_entropy values must
// produce different derived tokens.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_DifferentEntropy_ProducesDifferentDerivedTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  constexpr std::array<uint8_t, 4> entropy_a = {0xAA, 0xBB, 0xCC, 0xDD};
  constexpr std::array<uint8_t, 4> entropy_b = {0x11, 0x22, 0x33, 0x44};
  const auto t1 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), entropy_a);
  const auto t2 = brave_shields_settings()->GetFarblingToken(
      GURL("https://example.com"), entropy_b);
  EXPECT_NE(t1, t2);
}

// The base token (no entropy) is unaffected by what another origin's derived
// token looks like: each origin owns its own stored base token.
TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_PerOrigin_TokensAreIndependent) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));

  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_a = brave_shields_settings()->GetFarblingToken(
      GURL("https://a.com"), entropy);
  const auto derived_b = brave_shields_settings()->GetFarblingToken(
      GURL("https://b.com"), entropy);
  // Different origins → different base tokens → different derived tokens even
  // with identical entropy.
  EXPECT_NE(derived_a, derived_b);
}

TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_WithProfileEntropy_Behaviour) {
  // Set the profile token to a non-zero value.
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 1));
  const auto derived_a =
      brave_shields_settings()->GetFarblingToken(GURL("https://a.com"), {});

  // Simulating a different profile which will have a different non-zero value.
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 2));
  // We try and get the token to the same domain as above.
  const auto derived_a_new =
      brave_shields_settings()->GetFarblingToken(GURL("https://a.com"), {});

  if (IsFarblingTokenResetEnabled()) {
    // Different profiles should produce different tokens.
    EXPECT_NE(derived_a, derived_a_new);
  } else {
    // No profile level entropy is added.
    EXPECT_EQ(derived_a, derived_a_new);
  }
}

TEST_P(BraveShieldsSettingsFarblingTest,
       FarblingToken_WithProfileEntropy_WithAdditionalEntropy_Behaviour) {
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_a = brave_shields_settings()->GetFarblingToken(
      GURL("https://a.com"), {entropy});

  // Updating the profile level entropy to a different value.
  brave_shields_settings()->set_profile_level_farbling_entropy_for_testing(
      base::Token(0, 2));
  // Re-using the same entropy here, so as to fix the seed and entropy value to
  // check profile token changes, affects the farbling token when the feature
  // flag is enabled. Even though this case is very unlikely i.e 2 differnet
  // profiles sharing the same additional entropy, we are testing here the core
  // function so we should be ignorant about that.
  const auto derived_a_new = brave_shields_settings()->GetFarblingToken(
      GURL("https://a.com"), {entropy});

  if (IsFarblingTokenResetEnabled()) {
    // Different profiles should produce different tokens.
    EXPECT_NE(derived_a, derived_a_new);
  } else {
    EXPECT_EQ(derived_a, derived_a_new);
  }
}
