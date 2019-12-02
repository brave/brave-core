/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/surveyor_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SurveyorStateTest.*

namespace ledger {

TEST(SurveyorStateTest, ToJsonSerialization) {
  // Arrange
  SurveyorProperties surveyor_properties;
  surveyor_properties.signature = "Signature";
  surveyor_properties.surveyor_id = "SurveyorId";
  surveyor_properties.survey_vk = "SurveyVk";
  surveyor_properties.registrar_vk = "RegistrarVk";
  surveyor_properties.survey_sk = "SurveySk";

  // Act
  const SurveyorState surveyor_state;
  const std::string json = surveyor_state.ToJson(surveyor_properties);

  // Assert
  SurveyorProperties expected_surveyor_properties;
  surveyor_state.FromJson(json, &expected_surveyor_properties);
  EXPECT_EQ(expected_surveyor_properties, surveyor_properties);
}

TEST(SurveyorStateTest, FromJsonDeserialization) {
  // Arrange
  SurveyorProperties surveyor_properties;
  surveyor_properties.signature = "Signature";
  surveyor_properties.surveyor_id = "SurveyorId";
  surveyor_properties.survey_vk = "SurveyVk";
  surveyor_properties.registrar_vk = "RegistrarVk";
  surveyor_properties.survey_sk = "SurveySk";

  const std::string json = "{\"registrarVK\":\"RegistrarVk\",\"signature\":\"Signature\",\"surveyorId\":\"SurveyorId\",\"surveySK\":\"SurveySk\",\"surveyVK\":\"SurveyVk\"}";  // NOLINT

  // Act
  SurveyorProperties expected_surveyor_properties;
  const SurveyorState surveyor_state;
  surveyor_state.FromJson(json, &expected_surveyor_properties);

  // Assert
  EXPECT_EQ(expected_surveyor_properties, surveyor_properties);
}

}  // namespace ledger
