// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_shields {

class BraveShieldsP3ATest : public testing::Test {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        brave_shields::features::kBraveShredFeature);
    profile_ = std::make_unique<TestingProfile>();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
    RegisterShieldsP3ALocalPrefs(local_state_.registry());
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
    shields_settings_service_ = std::make_unique<BraveShieldsSettingsService>(
        *map, &local_state_, profile_->GetPrefs());
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveShieldsSettingsService> shields_settings_service_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
};

TEST_F(BraveShieldsP3ATest, RecordGlobalAdBlockSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://brave.com"));
  // Should not report to histogram if not a global change
  histogram_tester_->ExpectTotalCount(kAdsSettingHistogramName, 0);

  SetCosmeticFilteringControlType(map, ControlType::BLOCK, GURL());
  histogram_tester_->ExpectBucketCount(kAdsSettingHistogramName, 2, 1);

  SetCosmeticFilteringControlType(map, ControlType::BLOCK_THIRD_PARTY, GURL());
  histogram_tester_->ExpectBucketCount(kAdsSettingHistogramName, 1, 1);

  SetCosmeticFilteringControlType(map, ControlType::ALLOW, GURL());
  histogram_tester_->ExpectBucketCount(kAdsSettingHistogramName, 0, 1);

  histogram_tester_->ExpectTotalCount(kAdsSettingHistogramName, 3);
}

TEST_F(BraveShieldsP3ATest, RecordGlobalFingerprintBlockSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://brave.com"));
  // Should not report to histogram if not a global change
  histogram_tester_->ExpectTotalCount(kFingerprintSettingHistogramName, 0);

  SetFingerprintingControlType(map, ControlType::BLOCK, GURL());
  histogram_tester_->ExpectBucketCount(kFingerprintSettingHistogramName, 2, 1);

  SetFingerprintingControlType(map, ControlType::DEFAULT, GURL());
  histogram_tester_->ExpectBucketCount(kFingerprintSettingHistogramName, 1, 1);

  SetFingerprintingControlType(map, ControlType::ALLOW, GURL());
  histogram_tester_->ExpectBucketCount(kFingerprintSettingHistogramName, 0, 1);

  histogram_tester_->ExpectTotalCount(kFingerprintSettingHistogramName, 3);
}

TEST_F(BraveShieldsP3ATest, RecordDomainAdBlockCounts) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
  auto* prefs = profile_->GetPrefs();

  SetCosmeticFilteringControlType(map, ControlType::BLOCK_THIRD_PARTY, GURL());
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://brave.com"));

  // Test initial count
  MaybeRecordInitialShieldsSettings(
      &local_state_, profile_->GetPrefs(), map,
      GetCosmeticFilteringControlType(map, GURL()),
      GetFingerprintingControlType(map, GURL()));
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 1,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 0,
                                       1);

  // Test delta counting
  SetCosmeticFilteringControlType(map, ControlType::ALLOW,
                                  GURL("https://brave.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 0,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 1,
                                       1);

  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://yahoo.com"), nullptr, prefs);
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://reddit.com"), nullptr, prefs);
  SetCosmeticFilteringControlType(
      map, ControlType::BLOCK, GURL("https://craigslist.com"), nullptr, prefs);
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://github.com"), nullptr, prefs);
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://amazon.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 1,
                                       6);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 1,
                                       6);

  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://brave.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 2,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 0,
                                       2);

  // Change global setting
  SetCosmeticFilteringControlType(map, ControlType::BLOCK, GURL(), nullptr,
                                  prefs);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 0,
                                       2);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 0,
                                       3);

  SetCosmeticFilteringControlType(map, ControlType::BLOCK_THIRD_PARTY,
                                  GURL("https://amazon.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsAboveHistogramName, 0,
                                       3);
  histogram_tester_->ExpectBucketCount(kDomainAdsSettingsBelowHistogramName, 1,
                                       7);
}

TEST_F(BraveShieldsP3ATest, RecordDomainFingerprintBlockCounts) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_shields::features::kBraveShowStrictFingerprintingMode);

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
  auto* prefs = profile_->GetPrefs();

  SetFingerprintingControlType(map, ControlType::DEFAULT, GURL());
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://brave.com"));

  // Test initial count
  MaybeRecordInitialShieldsSettings(
      &local_state_, profile_->GetPrefs(), map,
      GetCosmeticFilteringControlType(map, GURL()),
      GetFingerprintingControlType(map, GURL()));
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 1,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 0,
                                       1);

  // Test delta counting
  SetFingerprintingControlType(map, ControlType::ALLOW,
                               GURL("https://brave.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 0,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 1,
                                       1);

  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://yahoo.com"), nullptr, prefs);
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://reddit.com"), nullptr, prefs);
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://craigslist.com"), nullptr, prefs);
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://github.com"), nullptr, prefs);
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://amazon.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 1,
                                       6);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 1,
                                       6);

  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://brave.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 2,
                                       1);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 0,
                                       2);

  // Change global setting
  SetFingerprintingControlType(map, ControlType::BLOCK, GURL(), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 0,
                                       2);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 0,
                                       3);

  SetFingerprintingControlType(map, ControlType::DEFAULT,
                               GURL("https://amazon.com"), nullptr, prefs);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsAboveHistogramName, 0,
                                       3);
  histogram_tester_->ExpectBucketCount(kDomainFPSettingsBelowHistogramName, 1,
                                       7);
}

TEST_F(BraveShieldsP3ATest, RecordHTTPSUpgradeGlobalSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());

  // Default is ASK (Standard) - should report 1
  RecordHTTPSUpgradeSettingP3A(map);
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSGlobalHistogramName, 1, 1);

  // Set to strict (BLOCK) - should report 2
  SetHttpsUpgradeControlType(map, ControlType::BLOCK, GURL());
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSGlobalHistogramName, 2, 1);

  // Set to disabled (ALLOW) - should suspend
  SetHttpsUpgradeControlType(map, ControlType::ALLOW, GURL());
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSGlobalHistogramName,
                                       INT_MAX - 1, 1);

  // Set back to standard (BLOCK_THIRD_PARTY) - should report 1 again
  SetHttpsUpgradeControlType(map, ControlType::BLOCK_THIRD_PARTY, GURL());
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSGlobalHistogramName, 1, 2);
}

TEST_F(BraveShieldsP3ATest, RecordHTTPSUpgradePerSiteNonDefault) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());

  // No per-site settings - should suspend
  RecordHTTPSUpgradeSettingP3A(map);
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSPerSiteHistogramName,
                                       INT_MAX - 1, 1);

  // Add a per-site strict (BLOCK) setting - differs from global, should report
  // 1
  SetHttpsUpgradeControlType(map, ControlType::BLOCK,
                             GURL("https://brave.com"));
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSPerSiteHistogramName, 1, 1);

  // Change to same as global (BLOCK_THIRD_PARTY) - matches global, should
  // suspend
  SetHttpsUpgradeControlType(map, ControlType::BLOCK_THIRD_PARTY,
                             GURL("https://brave.com"));
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSPerSiteHistogramName,
                                       INT_MAX - 1, 2);

  // Change to disabled (ALLOW) - differs from global, should report 1
  SetHttpsUpgradeControlType(map, ControlType::ALLOW,
                             GURL("https://brave.com"));
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSPerSiteHistogramName, 1, 2);
}

TEST_F(BraveShieldsP3ATest, RecordHTTPSUpgradeDisabledGlobalNoPerSite) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());

  // Set global to disabled - both metrics should suspend
  SetHttpsUpgradeControlType(map, ControlType::ALLOW, GURL());
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSGlobalHistogramName,
                                       INT_MAX - 1, 1);
  histogram_tester_->ExpectBucketCount(kUpgradeHTTPSPerSiteHistogramName,
                                       INT_MAX - 1, 1);
}

TEST_F(BraveShieldsP3ATest, AutoShredSettings_DisabledNoExceptions) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());

  ReportAutoShredSettingsP3A(*map);
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 0, 1);
}

TEST_F(BraveShieldsP3ATest, AutoShredSettings_DisabledWithEnabledExceptions) {
  shields_settings_service_->SetAutoShredMode(
      mojom::AutoShredMode::LAST_TAB_CLOSED, GURL("https://brave.com"));
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 1, 1);
}

TEST_F(BraveShieldsP3ATest, AutoShredSettings_EnabledNoExceptions) {
  shields_settings_service_->SetDefaultAutoShredMode(
      mojom::AutoShredMode::APP_EXIT);
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 2, 1);

  // A site with the same mode as global is not an exception.
  shields_settings_service_->SetAutoShredMode(mojom::AutoShredMode::APP_EXIT,
                                              GURL("https://brave.com"));
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 2, 2);
  histogram_tester_->ExpectTotalCount(kAutoShredSettingsHistogramName, 2);
}

TEST_F(BraveShieldsP3ATest, AutoShredSettings_EnabledWithDisabledExceptions) {
  shields_settings_service_->SetDefaultAutoShredMode(
      mojom::AutoShredMode::APP_EXIT);
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 2, 1);

  shields_settings_service_->SetAutoShredMode(mojom::AutoShredMode::NEVER,
                                              GURL("https://brave.com"));
  histogram_tester_->ExpectBucketCount(kAutoShredSettingsHistogramName, 3, 1);
  histogram_tester_->ExpectTotalCount(kAutoShredSettingsHistogramName, 2);
}

TEST_F(BraveShieldsP3ATest,
       AutoShredSettings_EnabledWithDifferentModeException) {
  shields_settings_service_->SetDefaultAutoShredMode(
      mojom::AutoShredMode::APP_EXIT);
  histogram_tester_->ExpectUniqueSample(kAutoShredSettingsHistogramName, 2, 1);

  // LAST_TAB_CLOSED differs from APP_EXIT global, so it is an exception.
  shields_settings_service_->SetAutoShredMode(
      mojom::AutoShredMode::LAST_TAB_CLOSED, GURL("https://brave.com"));
  histogram_tester_->ExpectBucketCount(kAutoShredSettingsHistogramName, 3, 1);
  histogram_tester_->ExpectTotalCount(kAutoShredSettingsHistogramName, 2);
}

TEST_F(BraveShieldsP3ATest, ManualShred_InitiallyFalse) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());
  MaybeRecordInitialShieldsSettings(
      &local_state_, profile_->GetPrefs(), map,
      GetCosmeticFilteringControlType(map, GURL()),
      GetFingerprintingControlType(map, GURL()));
  histogram_tester_->ExpectUniqueSample(kManualShredHistogramName, 0, 1);
}

TEST_F(BraveShieldsP3ATest, ManualShred_RecordedAfterTrigger) {
  RecordManualShredP3A(local_state_);
  histogram_tester_->ExpectUniqueSample(kManualShredHistogramName, 1, 1);
}

TEST_F(BraveShieldsP3ATest, ShredMetrics_NotReportedWhenFeatureDisabled) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndDisableFeature(
      brave_shields::features::kBraveShredFeature);

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_.get());

  ReportAutoShredSettingsP3A(*map);
  histogram_tester_->ExpectTotalCount(kAutoShredSettingsHistogramName, 0);

  RecordManualShredP3A(local_state_);
  histogram_tester_->ExpectTotalCount(kManualShredHistogramName, 0);
}

}  // namespace brave_shields
