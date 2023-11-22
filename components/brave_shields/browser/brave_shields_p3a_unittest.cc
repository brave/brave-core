/*  Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_shields {

class BraveShieldsP3ATest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  Profile* GetProfile() const { return profile_.get(); }

 protected:
  std::unique_ptr<base::HistogramTester> histogram_tester_;

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveShieldsP3ATest, RecordGlobalAdBlockSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(GetProfile());
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
  auto* map = HostContentSettingsMapFactory::GetForProfile(GetProfile());
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
  auto* map = HostContentSettingsMapFactory::GetForProfile(GetProfile());
  auto* prefs = GetProfile()->GetPrefs();

  SetCosmeticFilteringControlType(map, ControlType::BLOCK_THIRD_PARTY, GURL());
  SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                  GURL("https://brave.com"));

  // Test initial count
  MaybeRecordInitialShieldsSettings(GetProfile()->GetPrefs(), map);
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

  auto* map = HostContentSettingsMapFactory::GetForProfile(GetProfile());
  auto* prefs = GetProfile()->GetPrefs();

  SetFingerprintingControlType(map, ControlType::DEFAULT, GURL());
  SetFingerprintingControlType(map, ControlType::BLOCK,
                               GURL("https://brave.com"));

  // Test initial count
  MaybeRecordInitialShieldsSettings(GetProfile()->GetPrefs(), map);
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

}  // namespace brave_shields
