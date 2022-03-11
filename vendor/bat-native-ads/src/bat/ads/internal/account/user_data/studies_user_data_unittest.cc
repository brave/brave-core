/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/studies_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/metrics/field_trial.h"
#include "base/values.h"
#include "bat/ads/internal/features/features.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetStudiesAsJson() {
  const base::DictionaryValue user_data = user_data::GetStudies();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

TEST(BatAdsStudiesUserDataTest, GetStudiesForNoFieldTrials) {
  // Arrange

  // Act
  const std::string json = GetStudiesAsJson();

  // Assert
  const std::string expected_json = R"({"studies":[]})";

  EXPECT_EQ(expected_json, json);
}

TEST(BatAdsStudiesUserDataTest, GetStudies) {
  // Arrange
  std::string trial_name_1 = "BraveAdsFooStudy";
  std::string group_name_1 = "GroupA";
  scoped_refptr<base::FieldTrial> trial_1 =
      base::FieldTrialList::CreateFieldTrial(trial_name_1, group_name_1);
  trial_1->group();

  std::string trial_name_2 = "BarStudyForBraveAds";
  std::string group_name_2 = "GroupB";
  scoped_refptr<base::FieldTrial> trial_2 =
      base::FieldTrialList::CreateFieldTrial(trial_name_2, group_name_2);
  trial_2->group();

  std::string trial_name_3 = "FooBarStudy";
  std::string group_name_3 = "GroupC";
  scoped_refptr<base::FieldTrial> trial_3 =
      base::FieldTrialList::CreateFieldTrial(trial_name_3, group_name_3);
  trial_3->group();

  ASSERT_EQ(3U, base::FieldTrialList::GetFieldTrialCount());

  // Act
  const std::string json = GetStudiesAsJson();

  // Assert
  const std::string expected_json =
      R"({"studies":[{"group":"GroupB","name":"BarStudyForBraveAds"},{"group":"GroupA","name":"BraveAdsFooStudy"}]})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
