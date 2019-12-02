/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>

#include "bat/ledger/internal/state/transaction_ballot_state.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=TransactionBallotStateTest.*

namespace ledger {

TEST(TransactionBallotStateTest, ToJsonSerialization) {
  // Arrange
  TransactionBallotProperties transaction_ballot_properties;
  transaction_ballot_properties.publisher = "Publisher";
  transaction_ballot_properties.count =
      std::numeric_limits<uint32_t>::max();

  // Act
  const TransactionBallotState transaction_ballot_state;
  const std::string json =
      transaction_ballot_state.ToJson(transaction_ballot_properties);

  // Assert
  TransactionBallotProperties expected_transaction_ballot_properties;
  transaction_ballot_state.FromJson(json,
      &expected_transaction_ballot_properties);
  EXPECT_EQ(expected_transaction_ballot_properties,
      transaction_ballot_properties);
}

TEST(TransactionBallotStateTest, FromJsonDeserialization) {
  // Arrange
  TransactionBallotProperties transaction_ballot_properties;
  transaction_ballot_properties.publisher = "Publisher";
  transaction_ballot_properties.count =
      std::numeric_limits<uint32_t>::max();

  const std::string json = "{\"publisher\":\"Publisher\",\"offset\":4294967295}";  // NOLINT

  // Act
  TransactionBallotProperties expected_transaction_ballot_properties;
  const TransactionBallotState transaction_ballot_state;
  transaction_ballot_state.FromJson(json,
      &expected_transaction_ballot_properties);

  // Assert
  EXPECT_EQ(expected_transaction_ballot_properties,
      transaction_ballot_properties);
}

}  // namespace ledger
