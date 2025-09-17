// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_settings.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave_shields::mojom::AdBlockMode;
using brave_shields::mojom::FingerprintMode;

class BraveShieldsSettingsTest : public testing::Test {
 public:
  BraveShieldsSettingsTest() {}
  ~BraveShieldsSettingsTest() override = default;

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    brave_shields::RegisterShieldsP3AProfilePrefs(profile_prefs_.registry());
    brave_shields::RegisterShieldsP3ALocalPrefs(local_state_.registry());
    host_content_settings_map_ = new HostContentSettingsMap(
        &profile_prefs_, false /* is_off_the_record */,
        false /* store_last_modified */, false /* restore_session */,
        false /* should_record_metrics */);
    brave_shields_settings_ =
        std::make_unique<brave_shields::BraveShieldsSettings>(
            *GetHostContentSettingsMap(), GetLocalState(), &profile_prefs_);
  }

  void TearDown() override { host_content_settings_map_->ShutdownOnUIThread(); }

  TestingPrefServiceSimple* GetLocalState() { return &local_state_; }
  HostContentSettingsMap* GetHostContentSettingsMap() {
    return host_content_settings_map_.get();
  }

  const GURL kTestUrl{"https://brave.com"};

  brave_shields::BraveShieldsSettings* brave_shields_settings() {
    return brave_shields_settings_.get();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<brave_shields::BraveShieldsSettings> brave_shields_settings_;
};

TEST_F(BraveShieldsSettingsTest, BraveShieldsEnabled) {
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

TEST_F(BraveShieldsSettingsTest, AdBlockMode) {
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

TEST_F(BraveShieldsSettingsTest, DefaultAdBlockMode) {
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

TEST_F(BraveShieldsSettingsTest, FingerprintMode) {
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

TEST_F(BraveShieldsSettingsTest, DefaultFingerprintMode) {
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

TEST_F(BraveShieldsSettingsTest, NoScriptsEnabled) {
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

TEST_F(BraveShieldsSettingsTest, NoScriptsEnabledByDefault) {
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
