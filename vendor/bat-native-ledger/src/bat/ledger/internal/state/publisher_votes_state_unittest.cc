/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/publisher_votes_state.h"
#include "bat/ledger/internal/properties/transaction_ballot_properties.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PublisherVotesStateTest.*

namespace ledger {

TEST(PublisherVotesStateTest, ToJsonSerialization) {
  // Arrange
  PublisherVotesProperties publisher_votes_properties;
  publisher_votes_properties.publisher = "Publisher";

  PublisherVoteProperties publisher_vote_properties;
  publisher_vote_properties.surveyor_id = "SurveyorId";
  publisher_vote_properties.proof = "Proof";
  publisher_votes_properties.batch_votes.push_back(publisher_vote_properties);

  // Act
  const PublisherVotesState publisher_votes_state;
  const std::string json =
      publisher_votes_state.ToJson(publisher_votes_properties);

  // Assert
  PublisherVotesProperties expected_publisher_votes_properties;
  publisher_votes_state.FromJson(json, &expected_publisher_votes_properties);
  EXPECT_EQ(expected_publisher_votes_properties, publisher_votes_properties);
}

TEST(PublisherVotesStateTest, FromJsonDeserialization) {
  // Arrange
  PublisherVotesProperties publisher_votes_properties;
  publisher_votes_properties.publisher = "Publisher";

  PublisherVoteProperties publisher_vote_properties;
  publisher_vote_properties.surveyor_id = "SurveyorId";
  publisher_vote_properties.proof = "Proof";
  publisher_votes_properties.batch_votes.push_back(publisher_vote_properties);

  const std::string json = "{\"publisher\":\"Publisher\",\"batchVotesInfo\":[{\"surveyorId\":\"SurveyorId\",\"proof\":\"Proof\"}]}";  // NOLINT

  // Act
  PublisherVotesProperties expected_publisher_votes_properties;
  const PublisherVotesState publisher_votes_state;
  publisher_votes_state.FromJson(json, &expected_publisher_votes_properties);

  // Assert
  EXPECT_EQ(expected_publisher_votes_properties, publisher_votes_properties);
}

}  // namespace ledger
