/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/studies_user_data.h"

#include <string>

#include "base/metrics/field_trial.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

TEST(BatAdsStudiesUserDataTest, GetStudiesForNoFieldTrials) {
  // Arrange

  // Act
  const base::Value::Dict user_data = GetStudies();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"studies":[]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST(BatAdsStudiesUserDataTest, GetStudies) {
  // Arrange
  const std::string trial_name_1 = "BraveAdsFooStudy";
  const std::string group_name_1 = "GroupA";
  const scoped_refptr<base::FieldTrial> trial_1 =
      base::FieldTrialList::CreateFieldTrial(trial_name_1, group_name_1);
  trial_1->group_name();

  const std::string trial_name_2 = "BarStudyForBraveAds";
  const std::string group_name_2 = "GroupB";
  const scoped_refptr<base::FieldTrial> trial_2 =
      base::FieldTrialList::CreateFieldTrial(trial_name_2, group_name_2);
  trial_2->group_name();

  const std::string trial_name_3 = "FooBarStudy";
  const std::string group_name_3 = "GroupC";
  const scoped_refptr<base::FieldTrial> trial_3 =
      base::FieldTrialList::CreateFieldTrial(trial_name_3, group_name_3);
  trial_3->group_name();

  ASSERT_EQ(3U, base::FieldTrialList::GetFieldTrialCount());

  // Act
  const base::Value::Dict user_data = GetStudies();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"studies":[{"group":"GroupB","name":"BarStudyForBraveAds"},{"group":"GroupA","name":"BraveAdsFooStudy"}]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace ads::user_data
