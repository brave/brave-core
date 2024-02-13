// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/cookie_list_opt_in_service.h"

#include <memory>

#include "base/feature_list.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

class CookieListOptInServiceTest : public testing::Test {
 public:
  CookieListOptInServiceTest() = default;

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    RegisterPrefsForAdBlockService(registry);
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  PrefService* GetPrefs() { return &pref_service_; }

  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(CookieListOptInServiceTest, FeatureDisabledNoInitHistogram) {
  scoped_feature_list_.InitWithFeatures({}, {});
  CookieListOptInService service(nullptr, GetPrefs());
  // Should not write to histogram if feature is disabled
  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 0);
}

TEST_F(CookieListOptInServiceTest, FeatureEnabledInitHistogram) {
  scoped_feature_list_.InitWithFeatures(
      {brave_shields::features::kBraveAdblockCookieListOptIn}, {});
  CookieListOptInService service(nullptr, GetPrefs());
  // Should write to histogram if feature is enabled
  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 1);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 0, 1);
}

TEST_F(CookieListOptInServiceTest, FeatureEnabledShownNoInitHistogram) {
  scoped_feature_list_.InitWithFeatures(
      {brave_shields::features::kBraveAdblockCookieListOptIn}, {});
  GetPrefs()->SetBoolean(prefs::kAdBlockCookieListOptInShown, true);
  CookieListOptInService service(nullptr, GetPrefs());
  // Should not write to histogram if tooltip was already shown
  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 0);
}

TEST_F(CookieListOptInServiceTest, TooltipShownHistogram) {
  scoped_feature_list_.InitWithFeatures(
      {brave_shields::features::kBraveAdblockCookieListOptIn}, {});
  CookieListOptInService service(nullptr, GetPrefs());

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 1);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 0, 1);

  service.OnTooltipShown();

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 2);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 1, 1);
}

TEST_F(CookieListOptInServiceTest, TooltipNoClickedHistogram) {
  scoped_feature_list_.InitWithFeatures(
      {brave_shields::features::kBraveAdblockCookieListOptIn}, {});
  CookieListOptInService service(nullptr, GetPrefs());

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 1);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 0, 1);

  service.OnTooltipNoClicked();

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 2);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 2, 1);
}

TEST_F(CookieListOptInServiceTest, TooltipYesClickedHistogram) {
  scoped_feature_list_.InitWithFeatures(
      {brave_shields::features::kBraveAdblockCookieListOptIn}, {});
  CookieListOptInService service(nullptr, GetPrefs());

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 1);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 0, 1);

  service.OnTooltipYesClicked();

  histogram_tester_->ExpectTotalCount(kCookieListPromptHistogram, 2);
  histogram_tester_->ExpectBucketCount(kCookieListPromptHistogram, 3, 1);
}

}  // namespace brave_shields
