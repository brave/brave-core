/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/transaction_state.h"
#include "bat/ledger/internal/properties/transaction_ballot_properties.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=TransactionPropertiesTest.*

namespace ledger {

TEST(TransactionStateTest, ToJsonSerialization) {
  // Arrange
  TransactionProperties transaction_properties;
  transaction_properties.viewing_id = "ViewingId";
  transaction_properties.surveyor_id = "SurveyorId";
  transaction_properties.contribution_probi = "ContributionProbi";
  transaction_properties.submission_timestamp = "SubmissionTimestamp";
  transaction_properties.anonize_viewing_id = "AnonizeViewingId";
  transaction_properties.registrar_vk = "RegistrarVk";
  transaction_properties.master_user_token = "MasterUserToken";
  transaction_properties.surveyor_ids = { "SurveyorId" };
  transaction_properties.vote_count = std::numeric_limits<uint32_t>::max();
  transaction_properties.contribution_rates = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 }
  };

  TransactionBallotProperties transaction_ballot_properties;
  transaction_ballot_properties.publisher = "Publisher";
  transaction_ballot_properties.count =
      std::numeric_limits<uint32_t>::max();
  transaction_properties.transaction_ballots.push_back(
      transaction_ballot_properties);

  // Act
  const TransactionState transaction_state;
  const std::string json = transaction_state.ToJson(transaction_properties);

  // Assert
  TransactionProperties expected_transaction_properties;
  transaction_state.FromJson(json, &expected_transaction_properties);
  EXPECT_EQ(expected_transaction_properties, transaction_properties);
}

TEST(TransactionStateTest, FromJsonDeserialization) {
  // Arrange
  TransactionProperties transaction_properties;
  transaction_properties.viewing_id = "ViewingId";
  transaction_properties.surveyor_id = "SurveyorId";
  transaction_properties.contribution_probi = "ContributionProbi";
  transaction_properties.submission_timestamp = "SubmissionTimestamp";
  transaction_properties.anonize_viewing_id = "AnonizeViewingId";
  transaction_properties.registrar_vk = "RegistrarVk";
  transaction_properties.master_user_token = "MasterUserToken";
  transaction_properties.surveyor_ids = { "SurveyorId" };
  transaction_properties.vote_count = std::numeric_limits<uint32_t>::max();
  transaction_properties.contribution_rates = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 }
  };

  TransactionBallotProperties transaction_ballot_properties;
  transaction_ballot_properties.publisher = "Publisher";
  transaction_ballot_properties.count =
      std::numeric_limits<uint32_t>::max();
  transaction_properties.transaction_ballots.push_back(
      transaction_ballot_properties);

  const std::string json = "{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"contribution_probi\":\"ContributionProbi\",\"submissionStamp\":\"SubmissionTimestamp\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"surveyorIds\":[\"SurveyorId\"],\"votes\":4294967295,\"ballots\":[{\"publisher\":\"Publisher\",\"offset\":4294967295}]}";  // NOLINT

  // Act
  TransactionProperties expected_transaction_properties;
  const TransactionState transaction_state;
  transaction_state.FromJson(json, &expected_transaction_properties);

  // Assert
  EXPECT_EQ(expected_transaction_properties, transaction_properties);
}

TEST(TransactionStateTest, FromJsonResponseDeserialization) {
  // Arrange
  TransactionProperties transaction_properties;
  transaction_properties.contribution_probi = "Probi";
  transaction_properties.submission_timestamp = "1579627546681";

  const std::string json = "{\"probi\":\"Probi\",\"paymentStamp\":1579627546681}";  // NOLINT

  // Act
  TransactionProperties expected_transaction_properties;
  const TransactionState transaction_state;
  transaction_state.FromJsonResponse(json, &expected_transaction_properties);

  // Assert
  EXPECT_EQ(expected_transaction_properties, transaction_properties);
}

TEST(TransactionStateTest, FromInvalidJsonResponseDeserialization) {
  // Arrange
  TransactionProperties transaction_properties;
  transaction_properties.viewing_id = "ViewingId";
  transaction_properties.surveyor_id = "SurveyorId";
  transaction_properties.contribution_probi = "ContributionProbi";
  transaction_properties.submission_timestamp = "SubmissionTimestamp";
  transaction_properties.anonize_viewing_id = "AnonizeViewingId";
  transaction_properties.registrar_vk = "RegistrarVk";
  transaction_properties.master_user_token = "MasterUserToken";
  transaction_properties.surveyor_ids = { "SurveyorId" };
  transaction_properties.vote_count = std::numeric_limits<uint32_t>::max();
  transaction_properties.contribution_rates = {
    { "BAT", 1.0 },
    { "ETH", 2.0 },
    { "LTC", 3.0 },
    { "BTC", 4.0 },
    { "USD", 5.0 },
    { "EUR", 6.0 }
  };

  TransactionBallotProperties transaction_ballot_properties;
  transaction_ballot_properties.publisher = "Publisher";
  transaction_ballot_properties.count =
      std::numeric_limits<uint32_t>::max();
  transaction_properties.transaction_ballots.push_back(
      transaction_ballot_properties);

  const std::string json = "FOOBAR";

  // Act
  TransactionProperties expected_transaction_properties;
  const TransactionState transaction_state;
  transaction_state.FromJsonResponse(json, &expected_transaction_properties);

  // Assert
  EXPECT_NE(expected_transaction_properties, transaction_properties);
}

}  // namespace ledger
