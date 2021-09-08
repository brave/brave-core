/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/split_test_frequency_cap.h"

#include "base/metrics/field_trial.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kTrial[] = "AdvertiserSplitTestStudy";
const char kGroup[] = "GroupA";
const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";

scoped_refptr<base::FieldTrial> CreateFieldTrial(
    const std::string& trial_name) {
  return base::FieldTrialList::FactoryGetFieldTrial(
      trial_name, 100, "default", base::FieldTrial::ONE_TIME_RANDOMIZED,
      nullptr);
}

}  // namespace

class BatAdsSplitTestFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsSplitTestFrequencyCapTest() = default;

  ~BatAdsSplitTestFrequencyCapTest() override = default;
};

TEST_F(BatAdsSplitTestFrequencyCapTest,
    AllowIfNoFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  // Act
  SplitTestFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestFrequencyCapTest, DoNotAllowIfNoFieldTrialAndAdGroup) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.split_test_group = "GroupA";

  // Act
  SplitTestFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSplitTestFrequencyCapTest, AllowIfFieldTrialAndNoAdGroup) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  scoped_refptr<base::FieldTrial> trial = CreateFieldTrial(kTrial);
  trial->AppendGroup(kGroup, 100);

  // Act
  SplitTestFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestFrequencyCapTest, AllowIfFieldTrialMatchesAdGroup) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.split_test_group = "GroupA";

  scoped_refptr<base::FieldTrial> trial = CreateFieldTrial(kTrial);
  trial->AppendGroup(kGroup, 100);

  // Act
  SplitTestFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSplitTestFrequencyCapTest,
    DoNotAllowIfFieldTrialDoesNotMatchAdGroup) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.split_test_group = "GroupB";

  scoped_refptr<base::FieldTrial> trial = CreateFieldTrial(kTrial);
  trial->AppendGroup(kGroup, 100);

  // Act
  SplitTestFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
