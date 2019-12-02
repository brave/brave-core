/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/ballot_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BallotStateTest.*

namespace ledger {

TEST(BallotStateTest, ToJsonSerialization) {
  // Arrange
  BallotProperties ballot_properties;
  ballot_properties.viewing_id = "ViewingId";
  ballot_properties.surveyor_id = "SurveyorId";
  ballot_properties.publisher = "Publisher";
  ballot_properties.count = std::numeric_limits<uint32_t>::max();
  ballot_properties.prepare_ballot = "PrepareBallot";

  // Act
  const BallotState ballot_state;
  const std::string json = ballot_state.ToJson(ballot_properties);

  // Assert
  BallotProperties expected_ballot_properties;
  ballot_state.FromJson(json, &expected_ballot_properties);
  EXPECT_EQ(expected_ballot_properties, ballot_properties);
}

TEST(BallotStateTest, FromJsonDeserialization) {
  // Arrange
  BallotProperties ballot_properties;
  ballot_properties.viewing_id = "ViewingId";
  ballot_properties.surveyor_id = "SurveyorId";
  ballot_properties.publisher = "Publisher";
  ballot_properties.count = std::numeric_limits<uint32_t>::max();
  ballot_properties.prepare_ballot = "PrepareBallot";

  const std::string json = "{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"}";  // NOLINT

  // Act
  BallotProperties expected_ballot_properties;
  const BallotState ballot_state;
  ballot_state.FromJson(json, &expected_ballot_properties);

  // Assert
  EXPECT_EQ(expected_ballot_properties, ballot_properties);
}

}  // namespace ledger
