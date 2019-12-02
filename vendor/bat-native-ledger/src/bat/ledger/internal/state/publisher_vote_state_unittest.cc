/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/publisher_vote_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=VoteStateTest.*

namespace ledger {

TEST(VoteStateTest, ToJsonSerialization) {
  // Arrange
  PublisherVoteProperties vote_properties;
  vote_properties.surveyor_id = "SurveyorId";
  vote_properties.proof = "Proof";

  // Act
  const PublisherVoteState vote_state;
  const std::string json = vote_state.ToJson(vote_properties);

  // Assert
  PublisherVoteProperties expected_vote_properties;
  vote_state.FromJson(json, &expected_vote_properties);
  EXPECT_EQ(expected_vote_properties, vote_properties);
}

TEST(VoteStateTest, FromJsonDeserialization) {
  // Arrange
  PublisherVoteProperties vote_properties;
  vote_properties.surveyor_id = "SurveyorId";
  vote_properties.proof = "Proof";

  const std::string json = "{\"surveyorId\":\"SurveyorId\",\"proof\":\"Proof\"}";  // NOLINT

  // Act
  PublisherVoteProperties expected_vote_properties;
  const PublisherVoteState vote_state;
  vote_state.FromJson(json, &expected_vote_properties);

  // Assert
  EXPECT_EQ(expected_vote_properties, vote_properties);
}

}  // namespace ledger
