/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/studies_user_data.h"

#include "base/metrics/field_trial.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStudiesUserDataTest : public test::TestBase {};

TEST_F(BraveAdsStudiesUserDataTest, BuildStudiesUserDataIfNoFieldTrials) {
  // Arrange
  ASSERT_EQ(0U, base::FieldTrialList::GetFieldTrialCount());

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "studies": []
                    })JSON"),
            BuildStudiesUserData());
}

TEST_F(BraveAdsStudiesUserDataTest, BuildStudiesUserDataForSingleFieldTrial) {
  // Arrange
  const scoped_refptr<base::FieldTrial> field_trial =
      base::FieldTrialList::CreateFieldTrial("BraveAds.FooStudy", "GroupA");
  ASSERT_EQ("GroupA", field_trial->group_name());
  ASSERT_EQ(1U, base::FieldTrialList::GetFieldTrialCount());

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "studies": [
                        {
                          "group": "GroupA",
                          "name": "BraveAds.FooStudy"
                        }
                      ]
                    }
                )JSON"),
            BuildStudiesUserData());
}

TEST_F(BraveAdsStudiesUserDataTest,
       DoNotBuildStudiesUserDataForMultipleFieldTrials) {
  // Arrange
  const scoped_refptr<base::FieldTrial> field_trial_1 =
      base::FieldTrialList::CreateFieldTrial("BraveAds.FooStudy", "GroupA");
  ASSERT_EQ("GroupA", field_trial_1->group_name());

  const scoped_refptr<base::FieldTrial> field_trial_2 =
      base::FieldTrialList::CreateFieldTrial("BraveAds.BarStudy", "GroupB");
  ASSERT_EQ("GroupB", field_trial_2->group_name());

  ASSERT_EQ(2U, base::FieldTrialList::GetFieldTrialCount());

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "studies": []
                    })JSON"),
            BuildStudiesUserData());
}

TEST_F(BraveAdsStudiesUserDataTest,
       DoNotBuildStudiesUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const scoped_refptr<base::FieldTrial> field_trial =
      base::FieldTrialList::CreateFieldTrial("BraveAds.FooStudy", "GroupA");
  ASSERT_EQ("GroupA", field_trial->group_name());
  ASSERT_EQ(1U, base::FieldTrialList::GetFieldTrialCount());

  // Act & Assert
  EXPECT_THAT(BuildStudiesUserData(), ::testing::IsEmpty());
}

}  // namespace brave_ads
