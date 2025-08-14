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
            *host_content_settings_map_.get(), GetLocalState(),
            &profile_prefs_);
  }

  void TearDown() override { host_content_settings_map_->ShutdownOnUIThread(); }

  TestingPrefServiceSimple* GetLocalState() { return &local_state_; }

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
  EXPECT_TRUE(brave_shields_settings()->GetBraveShieldsEnabled(kTestUrl));
  brave_shields_settings()->SetBraveShieldsEnabled(false, kTestUrl);
  EXPECT_FALSE(brave_shields_settings()->GetBraveShieldsEnabled(kTestUrl));

  // verify other urls unchanged
  EXPECT_TRUE(brave_shields_settings()->GetBraveShieldsEnabled(
      GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsTest, AdBlockMode) {
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::STANDARD);
  brave_shields_settings()->SetAdBlockMode(AdBlockMode::AGGRESSIVE, kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::AGGRESSIVE);
  brave_shields_settings()->SetAdBlockMode(AdBlockMode::ALLOW, kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::ALLOW);

  // verify other urls remain unchanged
  EXPECT_EQ(
      brave_shields_settings()->GetAdBlockMode(GURL("https://example.com")),
      AdBlockMode::STANDARD);
}

TEST_F(BraveShieldsSettingsTest, DefaultAdBlockMode) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetAdBlockMode(AdBlockMode::STANDARD, kTestUrl);

  // test default adblock mode
  EXPECT_EQ(brave_shields_settings()->GetDefaultAdBlockMode(),
            AdBlockMode::STANDARD);
  brave_shields_settings()->SetDefaultAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(brave_shields_settings()->GetDefaultAdBlockMode(),
            AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(
      brave_shields_settings()->GetAdBlockMode(GURL("https://example.com")),
      AdBlockMode::AGGRESSIVE);

  // verify explict set adblock mode unchanged
  EXPECT_EQ(brave_shields_settings()->GetAdBlockMode(kTestUrl),
            AdBlockMode::STANDARD);
}

TEST_F(BraveShieldsSettingsTest, FingerprintMode) {
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::ALLOW_MODE,
                                               kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::ALLOW_MODE);

  // iOS does not support FingerprintMode::STRICT_MODE
#if !BUILDFLAG(IS_IOS)
  // when kBraveShowStrictFingerprintingMode flag is disabled
  // verify it returns FingerprintMode::STANDARD_MODE
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STRICT_MODE,
                                               kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
  // enable kBraveShowStrictFingerprintingMode flag
  // verify it returns FingerprintMode::STRICT_MODE
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_shields::features::kBraveShowStrictFingerprintingMode);
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STRICT_MODE,
                                               kTestUrl);
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STRICT_MODE);
#endif

  // verify other urls unchanged
  EXPECT_EQ(
      brave_shields_settings()->GetFingerprintMode(GURL("https://example.com")),
      FingerprintMode::STANDARD_MODE);
}

TEST_F(BraveShieldsSettingsTest, DefaultFingerprintMode) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetFingerprintMode(FingerprintMode::STANDARD_MODE,
                                               kTestUrl);

  // test default fingerprint mode
  EXPECT_EQ(brave_shields_settings()->GetDefaultFingerprintMode(),
            FingerprintMode::STANDARD_MODE);
  brave_shields_settings()->SetDefaultFingerprintMode(
      FingerprintMode::ALLOW_MODE);
  EXPECT_EQ(brave_shields_settings()->GetDefaultFingerprintMode(),
            FingerprintMode::ALLOW_MODE);
  EXPECT_EQ(
      brave_shields_settings()->GetFingerprintMode(GURL("https://example.com")),
      FingerprintMode::ALLOW_MODE);

  // verify explict set fingerprint mode is unchanged
  EXPECT_EQ(brave_shields_settings()->GetFingerprintMode(kTestUrl),
            FingerprintMode::STANDARD_MODE);
}

TEST_F(BraveShieldsSettingsTest, NoScriptsEnabled) {
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));
  brave_shields_settings()->SetNoScriptEnabled(true, kTestUrl);
  EXPECT_TRUE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));

  // verify other urls unchanged
  EXPECT_FALSE(
      brave_shields_settings()->IsNoScriptEnabled(GURL("https://example.com")));
}

TEST_F(BraveShieldsSettingsTest, NoScriptsEnabledByDefault) {
  // explicitly set so we can verify this is unchanged by updating default
  brave_shields_settings()->SetNoScriptEnabled(false, kTestUrl);

  // test default no script enabled
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabledByDefault());
  brave_shields_settings()->SetNoScriptEnabledByDefault(true);
  EXPECT_TRUE(brave_shields_settings()->IsNoScriptEnabledByDefault());
  EXPECT_TRUE(
      brave_shields_settings()->IsNoScriptEnabled(GURL("https://example.com")));

  // verify explict set no script enabled setting is unchanged
  EXPECT_FALSE(brave_shields_settings()->IsNoScriptEnabled(kTestUrl));
}
