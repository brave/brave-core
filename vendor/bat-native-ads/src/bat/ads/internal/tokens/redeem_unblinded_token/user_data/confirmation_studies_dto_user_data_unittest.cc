/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_studies_dto_user_data.h"

#include <string>
#include <utility>

#include "base/metrics/field_trial.h"
#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConfirmationStudiesDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationStudiesDtoUserDataTest() = default;

  ~BatAdsConfirmationStudiesDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationStudiesDtoUserDataTest, GetStudies) {
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
  base::DictionaryValue studies = dto::user_data::GetStudies();

  // Assert
  base::Value study_1(base::Value::Type::DICTIONARY);
  study_1.SetKey("name", base::Value("BraveAdsFooStudy"));
  study_1.SetKey("group", base::Value("GroupA"));

  base::Value study_2(base::Value::Type::DICTIONARY);
  study_2.SetKey("name", base::Value("BarStudyForBraveAds"));
  study_2.SetKey("group", base::Value("GroupB"));

  base::Value study_list(base::Value::Type::LIST);
  study_list.Append(std::move(study_2));
  study_list.Append(std::move(study_1));

  base::DictionaryValue expected_studies;
  expected_studies.SetKey("studies", std::move(study_list));

  EXPECT_EQ(expected_studies, studies);
}

}  // namespace ads
