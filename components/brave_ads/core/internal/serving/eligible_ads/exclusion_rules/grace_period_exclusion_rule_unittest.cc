/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/grace_period_exclusion_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr base::TimeDelta kGracePeriod = base::Days(3);
}  // namespace

class BraveAdsGracePeriodExclusionRuleTest : public test::TestBase {
 protected:
  void SetUpMocks() override {
    test::SetLocalStateTimePrefValue(prefs::kFirstRunAt, base::Time::Now());
    test::SetProfileTimeDeltaPrefValue(prefs::kGracePeriod, kGracePeriod);
  }

  const GracePeriodExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfWithinGracePeriodWhenDebugIsEnabled) {
  // Arrange
  ASSERT_TRUE(GlobalState::HasInstance());
  GlobalState::GetInstance()->Flags().should_debug = true;

  FastForwardClockBy(kGracePeriod - base::Milliseconds(1));

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfOutsideGracePeriodWhenDebugIsEnabled) {
  // Arrange
  ASSERT_TRUE(GlobalState::HasInstance());
  GlobalState::GetInstance()->Flags().should_debug = true;

  FastForwardClockBy(kGracePeriod);

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfWithinGracePeriodForDisabledMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod - base::Milliseconds(1));

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfOutsideGracePeriodForDisabledMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod);

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kDisabled;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldExcludeIfWithinGracePeriodForConfirmationMetrics) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldExcludeIfWithinGracePeriodOnTheCuspForConfirmationMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod - base::Milliseconds(1));

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfOutsideGracePeriodForConfirmationMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod);

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kConfirmation;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldExcludeIfWithinGracePeriodForP3aMetrics) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kP3A;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldExcludeIfOutsideGracePeriodOnTheCuspForP3aMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod - base::Milliseconds(1));

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kP3A;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsGracePeriodExclusionRuleTest,
       ShouldIncludeIfOutsideGracePeriodForP3aMetrics) {
  // Arrange
  FastForwardClockBy(kGracePeriod);

  CreativeAdInfo creative_ad;
  creative_ad.metric_type = mojom::NewTabPageAdMetricType::kP3A;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

}  // namespace brave_ads
