/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/split_test_exclusion_rule.h"

#include "base/metrics/field_trial.h"
#include "base/test/mock_entropy_provider.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kTrialName[] = "AdvertiserSplitTestStudy";
constexpr char kGroupName[] = "GroupA";

scoped_refptr<base::FieldTrial> CreateFieldTrial(
    const std::string& trial_name) {
  base::MockEntropyProvider entropy_provider(0.9);
  return base::FieldTrialList::FactoryGetFieldTrial(
      trial_name, /*total_probability=*/100, "default_group_name",
      entropy_provider);
}

}  // namespace

class BraveAdsSplitTestExclusionRuleTest : public test::TestBase {
 protected:
  const SplitTestExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsSplitTestExclusionRuleTest,
       ShouldIncludeIfNoFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsSplitTestExclusionRuleTest,
       ShouldExcludeIfNoFieldTrialAndAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.split_test_group = "GroupA";

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsSplitTestExclusionRuleTest,
       ShouldIncludeIfFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability=*/100);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsSplitTestExclusionRuleTest,
       ShouldIncludeIfFieldTrialMatchesAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.split_test_group = "GroupA";

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability=*/100);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsSplitTestExclusionRuleTest,
       ShouldExcludeIfFieldTrialDoesNotMatchAdGroup) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.split_test_group = "GroupB";

  const scoped_refptr<base::FieldTrial> field_trial =
      CreateFieldTrial(kTrialName);
  field_trial->AppendGroup(kGroupName, /*group_probability=*/100);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
