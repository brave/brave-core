/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/split_test_exclusion_rule.h"

#include "base/metrics/field_trial.h"
#include "base/test/mock_entropy_provider.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kTrialName[] = "AdvertiserSplitTestStudy";
constexpr char kGroupName[] = "GroupA";

scoped_refptr<base::FieldTrial> CreateFieldTrial(
    const std::string& trial_name) {
  base::MockEntropyProvider entropy_provider(0.9);
  return base::FieldTrialList::FactoryGetFieldTrial(
      trial_name, /*total_probability*/ 100, "default_group_name",
      entropy_provider);
}

}  // namespace

class BatAdsSplitTestExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsSplitTestExclusionRuleTest, AllowIfNoFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act
  SplitTestExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestExclusionRuleTest, DoNotAllowIfNoFieldTrialAndAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.split_test_group = "GroupA";

  // Act
  SplitTestExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSplitTestExclusionRuleTest, AllowIfFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability*/ 100);

  // Act
  SplitTestExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestExclusionRuleTest, AllowIfFieldTrialMatchesAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.split_test_group = "GroupA";

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability*/ 100);

  // Act
  SplitTestExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestExclusionRuleTest,
       DoNotAllowIfFieldTrialDoesNotMatchAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.split_test_group = "GroupB";

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability*/ 100);

  // Act
  SplitTestExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace brave_ads
