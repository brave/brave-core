/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/studies_user_data.h"

#include <string>

#include "base/metrics/field_trial.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::user_data {

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
  const std::string name_1 = "BraveAds.FooStudy";
  const std::string group_name_1 = "GroupA";
  const scoped_refptr<base::FieldTrial> field_trial_1 =
      base::FieldTrialList::CreateFieldTrial(name_1, group_name_1);
  field_trial_1->group_name();

  const std::string name_2 = "BraveAds.BarStudy";
  const std::string group_name_2 = "GroupB";
  const scoped_refptr<base::FieldTrial> field_trial_2 =
      base::FieldTrialList::CreateFieldTrial(name_2, group_name_2);
  field_trial_2->group_name();

  const std::string name_3 = "FooBarStudy";
  const std::string group_name_3 = "GroupC";
  const scoped_refptr<base::FieldTrial> field_trial_3 =
      base::FieldTrialList::CreateFieldTrial(name_3, group_name_3);
  field_trial_3->group_name();

  ASSERT_EQ(3U, base::FieldTrialList::GetFieldTrialCount());

  // Act
  const base::Value::Dict user_data = GetStudies();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"studies":[{"group":"GroupB","name":"BraveAds.BarStudy"},{"group":"GroupA","name":"BraveAds.FooStudy"}]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads::user_data
